#include "ToolSwap.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// The references looked at each second.
		constexpr float kSearch = 600.0f;
		// A vein is offered from this close to its bounds, facing it within the cone. Veins are wide
		// and their origin sits inside the rock, so the edge is what counts.
		constexpr float kVeinReach = 200.0f;
		constexpr float kVeinCone = 60.0f;
		// A tree is offered from this close to its trunk (the reference's origin), facing it. Its
		// bounds are the canopy, which would put the player "at" a tree anywhere under it.
		constexpr float kTreeReach = 250.0f;
		constexpr float kTreeCone = 45.0f;
		// SI tells trees from plants by height (TreeWeaponSwap::IsTree logs one). Skyrim.esm's TREE
		// records split cleanly around this: flora, shrubs, ferns and kelp top out below 400 units,
		// pines, aspens and Reach trees stand 676-2501. Flora with an ingredient is skipped outright.
		constexpr float kTreeHeight = 450.0f;
		// Facing away from every vein and tree for this long offers the way back.
		constexpr auto kAwayDelay = 2s;
		constexpr auto kCheckDelay = 1500ms;
		constexpr RE::FormID kWoodAxesID = 0x10ACCC;  // woodChoppingAxes, Skyrim.esm

		// Degrees between the player's facing and the point (a_x, a_y) relative to the player.
		float AngleTo(float a_yaw, float a_x, float a_y)
		{
			// Skyrim yaw: 0 faces +Y, growing clockwise; atan2(x, y) gives the same convention.
			float angle = std::fmod(std::abs(RE::rad_to_deg(std::atan2(a_x, a_y) - a_yaw)), 360.0f);
			return angle > 180.0f ? 360.0f - angle : angle;
		}

		// The closest point of a reference's rotated bounds, seen from above, relative to a_origin
		// (the same construction Rest uses for fires).
		RE::NiPoint2 ClosestPoint(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_origin)
		{
			const auto center = a_ref->GetPosition() - a_origin;
			const float rot = a_ref->GetAngleZ();
			const float cs = std::cos(rot);
			const float sn = std::sin(rot);
			const float scale = a_ref->GetScale();
			const auto lo = a_ref->GetBoundMin();
			const auto hi = a_ref->GetBoundMax();
			const float dx = -center.x;
			const float dy = -center.y;
			const float lx = std::clamp(dx * cs - dy * sn, lo.x * scale, hi.x * scale);
			const float ly = std::clamp(dx * sn + dy * cs, lo.y * scale, hi.y * scale);
			return { center.x + lx * cs + ly * sn, center.y - lx * sn + ly * cs };
		}

		bool OneHanded(const RE::TESObjectWEAP* a_weapon)
		{
			return a_weapon->IsOneHandedSword() || a_weapon->IsOneHandedDagger() || a_weapon->IsOneHandedAxe() ||
			       a_weapon->IsOneHandedMace() || a_weapon->IsStaff();
		}

		RE::BGSEquipSlot* RightSlot()
		{
			auto* defaults = RE::BGSDefaultObjectManager::GetSingleton();
			return defaults ? defaults->GetObject<RE::BGSEquipSlot>(RE::DEFAULT_OBJECT::kRightHandEquip) : nullptr;
		}
	}

	ToolSwap::ToolSwap()
	{
		take.SetPromptType(SkyPromptAPI::kHold);
		// A plain press: the way back is also offered in combat, where a hold would cost the fight.
		giveBack.SetRepeat(false);
	}

	ToolSwap* ToolSwap::GetSingleton()
	{
		static ToolSwap singleton;
		return &singleton;
	}

	const char* ToolSwap::KindTag(Kind a_kind)
	{
		switch (a_kind) {
		case Kind::kVein:
			return "vein";
		case Kind::kTree:
			return "tree";
		default:
			return "-";
		}
	}

	void ToolSwap::OnGameLoaded()
	{
		take.Reset();
		giveBack.Reset();
		lastGate.clear();
		offeredKind = Kind::kNone;
		offeredTool = nullptr;
		swapped = false;
		savedRight = nullptr;
		heldTool = nullptr;
		away = false;
		checkPending = false;
		woodAxes = RE::TESForm::LookupByID<RE::BGSListForm>(kWoodAxesID);
		Util::WarnIfSIModuleOn("WeaponSwap.enabled", "/MCP/modules/WeaponSwap/enabled");
		Log("ready: woodChoppingAxes={} ({} forms)", woodAxes != nullptr, woodAxes ? woodAxes->forms.size() : 0);
	}

	ToolSwap::Scan ToolSwap::Look(RE::PlayerCharacter* a_player) const
	{
		Scan scan;
		const auto origin = a_player->GetPosition();
		const float yaw = a_player->GetAngleZ();
		Target vein;
		Target tree;
		RE::TES::GetSingleton()->ForEachReferenceInRange(a_player, kSearch, [&](RE::TESObjectREFR* a_ref) {
			auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
			if (!base || a_ref->IsDisabled() || a_ref->IsDeleted()) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			if (base->Is(RE::FormType::Tree)) {
				const auto* treeBase = base->As<RE::TESObjectTREE>();
				if (!treeBase || treeBase->produceItem || !woodAxes ||
					a_ref->GetBoundMax().z * a_ref->GetScale() < kTreeHeight) {
					return RE::BSContainer::ForEachResult::kContinue;
				}
				++scan.trees;
				const auto at = a_ref->GetPosition() - origin;
				const float distance = std::hypot(at.x, at.y);
				const float angle = AngleTo(yaw, at.x, at.y);
				if (distance <= kTreeReach && angle <= kTreeCone && (!tree.ref || distance < tree.distance)) {
					tree = { Kind::kTree, a_ref, woodAxes, distance, angle };
				}
				return RE::BSContainer::ForEachResult::kContinue;
			}
			if (!base->Is(RE::FormType::Activator)) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			const auto script = Util::ScriptObject(a_ref, "MineOreScript");
			if (!script) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			++scan.veins;
			// MineOreScript: ResourceCountCurrent 0 is a depleted vein (-1 until first mined).
			if (Util::ScriptInt(script, "ResourceCountCurrent", -1) == 0) {
				++scan.depleted;
				return RE::BSContainer::ForEachResult::kContinue;
			}
			auto* tools = Util::ScriptProperty<RE::BGSListForm>(script, "mineOreToolsList");
			if (!tools) {
				return RE::BSContainer::ForEachResult::kContinue;
			}
			const auto point = ClosestPoint(a_ref, origin);
			const float edge = std::hypot(point.x, point.y);
			const float angle = edge < 1.0f ? 0.0f : AngleTo(yaw, point.x, point.y);
			if (edge <= kVeinReach && angle <= kVeinCone && (!vein.ref || edge < vein.distance)) {
				vein = { Kind::kVein, a_ref, tools, edge, angle };
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
		// A vein is where the tool does something, so it wins over a tree beside it.
		scan.target = vein.ref ? vein : tree;
		return scan;
	}

	RE::TESObjectWEAP* ToolSwap::PickTool(RE::PlayerCharacter* a_player, RE::BGSListForm* a_tools)
	{
		if (!a_tools) {
			return nullptr;
		}
		RE::TESObjectWEAP* best = nullptr;
		bool bestFavorite = false;
		float bestDamage = 0.0f;
		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::Weapon); });
		for (const auto& [object, data] : inventory) {
			auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr;
			if (!weapon || data.first <= 0 || !a_tools->HasForm(weapon)) {
				continue;
			}
			const bool favorite = data.second && data.second->IsFavorited();
			const float damage = static_cast<float>(weapon->GetAttackDamage());
			if (!best || (favorite && !bestFavorite) || (favorite == bestFavorite && damage > bestDamage)) {
				best = weapon;
				bestFavorite = favorite;
				bestDamage = damage;
			}
		}
		return best;
	}

	void ToolSwap::Tick()
	{
		Util::WarnIfSIModuleOn("WeaponSwap.enabled", "/MCP/modules/WeaponSwap/enabled");
		auto* player = Util::Player();
		auto* right = player->GetEquippedObject(false);
		const auto now = Clock::now();

		if (checkPending && now >= checkAt) {
			checkPending = false;
			Log("after swap: right hand {} (expected {})", right ? Util::NameOf(right) : "empty"s,
				expectedRight ? Util::NameOf(expectedRight) : "empty"s);
			if (right != expectedRight) {
				Log("WARN the right hand is not what was equipped");
				Util::Notify("CIGAR: 도구 교체 결과 다름. 로그 확인");
			}
		}

		// The player put something else in the right hand: the tool is not CIGAR's to give back.
		if (swapped && !checkPending && right != heldTool) {
			Log("right hand changed by the player ({}); forgetting {}", right ? Util::NameOf(right) : "empty"s,
				savedRight ? Util::NameOf(savedRight) : "empty hand"s);
			swapped = false;
			savedRight = nullptr;
			heldTool = nullptr;
			giveBack.Withdraw();
			giveBack.Reset();
		}

		const bool combat = player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const auto* state = player->AsActorState();
		const bool seated = state && state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal;
		const bool busy = player->IsOnMount() || seated || !movable;

		const auto scan = busy ? Scan{} : Look(player);
		const auto& target = scan.target;

		// Already holding a tool that serves this target: nothing to offer.
		auto* rightWeapon = right ? right->As<RE::TESObjectWEAP>() : nullptr;
		const bool holdsTool = target.tools && rightWeapon && target.tools->HasForm(rightWeapon);
		auto* tool = target.ref && !holdsTool && !combat && !busy ? PickTool(player, target.tools) : nullptr;

		if (target.ref) {
			away = false;
		} else if (!away) {
			away = true;
			awaySince = now;
		}
		const bool leaving = swapped && (combat || (away && now - awaySince >= kAwayDelay));

		LogGate(std::format("target={} {} dist={:.0f} angle={:.0f} veins={} depleted={} trees={} right={} holdsTool={} tool={} combat={} busy={} swapped={} leaving={}",
			KindTag(target.kind), target.ref ? std::format("{:08X}", target.ref->GetFormID()) : "-"s, target.distance, target.angle,
			scan.veins, scan.depleted, scan.trees, right ? Util::NameOf(right) : "empty"s, holdsTool,
			tool ? Util::NameOf(tool) : "-"s, combat, busy, swapped, leaving));

		// The prompt names the tool and says what it is for, so a change offers it again.
		if (take.Offered() && (target.kind != offeredKind || tool != offeredTool)) {
			take.Withdraw();
			take.Reset();
		}
		offeredKind = tool ? target.kind : Kind::kNone;
		offeredTool = tool;
		const bool vein = target.kind == Kind::kVein;
		take.Update(tool != nullptr, [tool, vein] {
			return std::format("{} (길게): {}", vein ? "곡괭이 들기" : "도끼 들기", Util::NameOf(tool));
		});

		auto* saved = savedRight;
		giveBack.Update(leaving && movable, [saved] {
			return std::format("무기 되돌리기: {}", saved ? Util::NameOf(saved) : "맨손"s);
		});
	}

	void ToolSwap::OnAccepted(std::uint16_t a_eventID)
	{
		auto* player = Util::Player();
		if (a_eventID == kReturn) {
			Restore(player);
			return;
		}
		if (a_eventID != kTake) {
			return;
		}
		if (player->IsInCombat()) {
			Log("accept ignored: in combat");
			return;
		}
		// Re-read: the player may have turned away or dropped the tool while the prompt was up.
		const auto scan = Look(player);
		auto* tool = scan.target.ref ? PickTool(player, scan.target.tools) : nullptr;
		if (!tool) {
			Log("accept ignored: target={} tool=none", KindTag(scan.target.kind));
			return;
		}
		auto* right = player->GetEquippedObject(false);
		// A second swap (axe, then pickaxe) keeps the hand held before the first.
		if (!swapped) {
			savedRight = right;
		}
		auto* manager = RE::ActorEquipManager::GetSingleton();
		manager->EquipObject(player, tool, nullptr, 1, OneHanded(tool) ? RightSlot() : nullptr);
		swapped = true;
		heldTool = tool;
		expectedRight = tool;
		checkPending = true;
		checkAt = Clock::now() + kCheckDelay;
		take.Withdraw();
		take.Reset();
		Log("took {} ({:08X}) for {} {:08X} at {:.0f} units; saved right hand {}", Util::NameOf(tool), tool->GetFormID(),
			KindTag(scan.target.kind), scan.target.ref->GetFormID(), scan.target.distance, savedRight ? Util::NameOf(savedRight) : "empty"s);
	}

	void ToolSwap::Restore(RE::PlayerCharacter* a_player)
	{
		if (!swapped) {
			Log("return ignored: nothing to give back");
			return;
		}
		auto* manager = RE::ActorEquipManager::GetSingleton();
		auto* saved = savedRight;
		RE::TESForm* expected = nullptr;
		if (auto* spell = saved ? saved->As<RE::SpellItem>() : nullptr) {
			if (a_player->HasSpell(spell)) {
				manager->EquipSpell(a_player, spell, RightSlot());
				expected = spell;
			}
		} else if (auto* weapon = saved ? saved->As<RE::TESObjectWEAP>() : nullptr) {
			if (Util::ItemCount(a_player, weapon) > 0) {
				manager->EquipObject(a_player, weapon, nullptr, 1, OneHanded(weapon) ? RightSlot() : nullptr);
				expected = weapon;
			}
		}
		if (!expected && heldTool) {
			// Nothing in hand before, or it is gone: put the tool away.
			manager->UnequipObject(a_player, heldTool, nullptr, 1, RightSlot());
		}
		Log("gave back {}{}", saved ? Util::NameOf(saved) : "empty hand"s,
			saved && !expected ? " (no longer carried or known; tool put away)" : "");
		swapped = false;
		savedRight = nullptr;
		heldTool = nullptr;
		expectedRight = expected;
		checkPending = true;
		checkAt = Clock::now() + kCheckDelay;
		giveBack.Withdraw();
		giveBack.Reset();
	}

	void ToolSwap::OnDisabled()
	{
		swapped = false;
		savedRight = nullptr;
		heldTool = nullptr;
		checkPending = false;
	}
}
