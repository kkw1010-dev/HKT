#include "MannequinSwap.h"

#include "Dress.h"
#include "Helmet.h"
#include "PartyOutfit.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kScript = "MannequinActivatorSCRIPT";
		// ManikinRace: every mannequin actor in the base game, the DLCs and the Creation Club homes.
		// Only a pre-filter; the script decides (docs/030, "Which actors are mannequins").
		constexpr RE::FormID kManikinRaceID = 0x10760A;
		constexpr auto kSkyrimPlugin = "Skyrim.esm"sv;

		// The crosshair usually lands on the MannequinActivateTrig activator in front of the actor.
		constexpr float kNearCrosshair = 200.0f;
		constexpr float kReach = 300.0f;
		constexpr float kDismissDistance = 300.0f;
		constexpr auto kScriptWait = 3s;
		constexpr auto kSettleWait = 1s;
		// Another Mannequin Script Fix has 20 properties; vanilla 10.
		constexpr int kMaxPropertySlots = 20;

		RE::BGSBipedObjectForm::BipedObjectSlot SlotMask(std::uint32_t a_slot)
		{
			return static_cast<RE::BGSBipedObjectForm::BipedObjectSlot>(1u << (a_slot - 30));
		}

		// HDT SMP Object's carriers (HDTSMPObjectBase and its collision variants): non-playable, slot 60.
		bool IsSmpCarrier(const RE::TESObjectARMO* a_armor)
		{
			const auto* file = a_armor && !a_armor->GetPlayable() ? a_armor->GetFile(0) : nullptr;
			return file && Util::ContainsNoCase(file->GetFilename(), "HDT SMP Object");
		}

		RE::TESForm* FormOf(const RE::BSScript::Variable* a_var)
		{
			// An element whose declared type is unlinked holds garbage (Util::ScriptProperty, CTD 2026-09-29).
			return a_var && a_var->IsObject() && Util::LinkedAs(a_var->GetType().GetTypeInfo(), nullptr) ? a_var->Unpack<RE::TESForm*>() : nullptr;
		}
	}

	std::size_t MannequinSwap::Slots::Occupied() const
	{
		return static_cast<std::size_t>(std::ranges::count_if(forms, [this](const RE::TESForm* a_form) { return !Free(a_form); }));
	}

	std::size_t MannequinSwap::Slots::CountOf(const RE::TESForm* a_form) const
	{
		return static_cast<std::size_t>(std::ranges::count(forms, a_form));
	}

	MannequinSwap::MannequinSwap()
	{
		swap.SetPromptType(SkyPromptAPI::kHold);
	}

	MannequinSwap* MannequinSwap::GetSingleton()
	{
		static MannequinSwap singleton;
		return &singleton;
	}

	void MannequinSwap::OnGameLoaded()
	{
		swap.Reset();
		lastGate.clear();
		Reset();
		dismissedFor = {};
		current = {};
		notifiedBlock.clear();
		auto* handler = RE::TESDataHandler::GetSingleton();
		manikinRace = handler ? handler->LookupForm<RE::TESRace>(kManikinRaceID, kSkyrimPlugin) : nullptr;
		armorHelmet = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ArmorHelmet");
		clothingHead = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ClothingHead");
		clothingCirclet = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ClothingCirclet");
		Log("ready: manikinRace={} keywords helmet={} head={} circlet={}", manikinRace != nullptr, armorHelmet != nullptr,
			clothingHead != nullptr, clothingCirclet != nullptr);
	}

	void MannequinSwap::Reset()
	{
		phase = Phase::kIdle;
		target = {};
		give.clear();
		take.clear();
		given.clear();
		taken.clear();
		stowedHelmets.clear();
		countsBefore.clear();
		slotCountsBefore.clear();
	}

	bool MannequinSwap::IsMannequin(RE::Actor* a_actor) const
	{
		if (!a_actor || (manikinRace && a_actor->GetRace() != manikinRace)) {
			return false;
		}
		return static_cast<bool>(Util::ScriptObject(a_actor, kScript));
	}

	RE::Actor* MannequinSwap::FindMannequin(RE::PlayerCharacter* a_player, std::string& a_how) const
	{
		auto* pick = RE::CrosshairPickData::GetSingleton();
		const auto ref = pick ? pick->GetActiveTarget().get() : RE::TESObjectREFRPtr{};
		if (!ref) {
			return nullptr;
		}
		RE::Actor* found = nullptr;
		if (auto* actor = ref->As<RE::Actor>(); actor && IsMannequin(actor)) {
			found = actor;
			a_how = "crosshair";
		} else if (auto* tes = RE::TES::GetSingleton()) {
			float best = kNearCrosshair;
			const auto origin = ref->GetPosition();
			tes->ForEachReferenceInRange(ref.get(), kNearCrosshair, [&](RE::TESObjectREFR* a_ref) {
				auto* actor = a_ref ? a_ref->As<RE::Actor>() : nullptr;
				if (actor && IsMannequin(actor)) {
					const float distance = origin.GetDistance(actor->GetPosition());
					if (distance <= best) {
						best = distance;
						found = actor;
					}
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
			if (found) {
				a_how = std::format("near {} ({:08X})", Util::NameOf(ref.get()), ref->GetFormID());
			}
		}
		if (found && a_player->GetPosition().GetDistance(found->GetPosition()) > kReach) {
			return nullptr;
		}
		return found;
	}

	MannequinSwap::Slots MannequinSwap::ReadSlots(RE::Actor* a_mannequin) const
	{
		Slots slots;
		const auto object = Util::ScriptObject(a_mannequin, kScript);
		if (!object) {
			return slots;
		}
		slots.empty = Util::ScriptProperty<RE::TESForm>(object, "EmptySlot");
		// USSEP keeps the slots in a variable array once ConvertArmorSlots() has run (it empties the
		// old properties then); before that, and in the vanilla script and the fix, the properties hold them.
		if (const auto* var = object->GetVariable("ArmorSlot"); var && var->IsArray()) {
			if (const auto array = var->GetArray(); array && array->size() > 0) {
				for (std::uint32_t i = 0; i < array->size(); ++i) {
					slots.forms.push_back(FormOf(&(*array)[i]));
				}
				slots.variant = "array";
				slots.readable = true;
				return slots;
			}
		}
		for (int n = 1; n <= kMaxPropertySlots; ++n) {
			const auto* var = object->GetProperty(std::format("ArmorSlot{:02}", n));
			if (!var) {
				break;
			}
			slots.forms.push_back(FormOf(var));
		}
		slots.variant = "properties";
		slots.readable = !slots.forms.empty();
		return slots;
	}

	bool MannequinSwap::IsHead(const RE::TESObjectARMO* a_armor) const
	{
		if (!a_armor || (clothingCirclet && a_armor->HasKeyword(clothingCirclet))) {
			return false;
		}
		return (armorHelmet && a_armor->HasKeyword(armorHelmet)) || (clothingHead && a_armor->HasKeyword(clothingHead));
	}

	MannequinSwap::Piece MannequinSwap::MakePiece(RE::TESObjectARMO* a_armor, RE::ExtraDataList* a_extra) const
	{
		Piece piece{ a_armor, a_extra, IsHead(a_armor), std::format("{} ({:08X})", Util::NameOf(a_armor), a_armor->GetFormID()) };
		if (!a_extra) {
			piece.text += " plain";
			return piece;
		}
		if (const auto* ench = a_extra->GetByType<RE::ExtraEnchantment>(); ench && ench->enchantment) {
			piece.text += std::format(" ench={}", Util::NameOf(ench->enchantment));
		}
		if (const auto* health = a_extra->GetByType<RE::ExtraHealth>()) {
			piece.text += std::format(" temper={:.2f}", health->health);
		}
		if (const auto* charge = a_extra->GetByType<RE::ExtraCharge>()) {
			piece.text += std::format(" charge={:.0f}", charge->charge);
		}
		return piece;
	}

	RE::ExtraDataList* MannequinSwap::WornExtra(RE::Actor* a_actor, RE::TESBoundObject* a_item)
	{
		auto* changes = a_actor ? a_actor->GetInventoryChanges() : nullptr;
		if (!changes || !changes->entryList) {
			return nullptr;
		}
		for (auto* entry : *changes->entryList) {
			if (!entry || entry->object != a_item || !entry->extraLists) {
				continue;
			}
			for (auto* extra : *entry->extraLists) {
				if (extra && (extra->HasType(RE::ExtraDataType::kWorn) || extra->HasType(RE::ExtraDataType::kWornLeft))) {
					return extra;
				}
			}
		}
		return nullptr;
	}

	RE::ExtraDataList* MannequinSwap::AnyExtra(RE::Actor* a_actor, RE::TESBoundObject* a_item)
	{
		auto* changes = a_actor ? a_actor->GetInventoryChanges() : nullptr;
		if (!changes || !changes->entryList) {
			return nullptr;
		}
		for (auto* entry : *changes->entryList) {
			if (entry && entry->object == a_item && entry->extraLists) {
				for (auto* extra : *entry->extraLists) {
					if (extra) {
						return extra;
					}
				}
			}
		}
		return nullptr;
	}

	bool MannequinSwap::HasExtra(RE::Actor* a_actor, RE::TESBoundObject* a_item, const RE::ExtraDataList* a_extra)
	{
		auto* changes = a_actor && a_extra ? a_actor->GetInventoryChanges() : nullptr;
		if (!changes || !changes->entryList) {
			return false;
		}
		for (auto* entry : *changes->entryList) {
			if (entry && entry->object == a_item && entry->extraLists) {
				for (auto* extra : *entry->extraLists) {
					if (extra == a_extra) {
						return true;
					}
				}
			}
		}
		return false;
	}

	bool MannequinSwap::IsWearing(RE::Actor* a_actor, RE::TESObjectARMO* a_armor)
	{
		return a_actor && a_armor && a_actor->GetWornArmor(a_armor->GetFormID()) != nullptr;
	}

	std::vector<RE::TESObjectARMO*> MannequinSwap::WornOutfit(RE::Actor* a_actor, std::string* a_skipped) const
	{
		// Every worn armor once, from the inventory, not slot by slot (Util::GetStrippable asks each
		// slot for its piece). Headgear moves on any slot: a helmet on hair + circlet (31, 42) stayed
		// behind in test 2 and again in test 3 (2026-09-26). Otherwise a piece moves when any of its
		// slots may be stripped. Non-playable items (the SMP carrier), no-strip items, shields and
		// TCL lanterns (a light, not clothing) stay; a_skipped says which and why.
		std::vector<RE::TESObjectARMO*> worn;
		auto* changes = a_actor ? a_actor->GetInventoryChanges() : nullptr;
		if (!changes || !changes->entryList) {
			return worn;
		}
		const auto skip = [a_skipped](const RE::TESObjectARMO* a_armor, std::string_view a_why) {
			if (a_skipped) {
				*a_skipped += std::format(" {}:{} ({:08X})", Util::NameOf(a_armor), a_why, a_armor->GetSlotMask().underlying());
			}
		};
		for (auto* entry : *changes->entryList) {
			auto* armor = entry && entry->object ? entry->object->As<RE::TESObjectARMO>() : nullptr;
			if (!armor || !entry->IsWorn() || std::ranges::find(worn, armor) != worn.end()) {
				continue;
			}
			if (armor->IsShield()) {
				skip(armor, "shield");
				continue;
			}
			if (const auto* file = armor->GetFile(0); file && Util::ContainsNoCase(file->GetFilename(), "TorchesCandlelightLanterns")) {
				skip(armor, "lantern");
				continue;
			}
			if (!armor->GetPlayable()) {
				skip(armor, "non-playable");
				continue;
			}
			// Slot 32 is never a kept slot, so a false here is a no-strip keyword.
			if (!Util::IsStrippable(armor, 32)) {
				skip(armor, "no-strip keyword");
				continue;
			}
			if (IsHead(armor)) {
				worn.push_back(armor);
				continue;
			}
			const auto mask = armor->GetSlotMask().underlying();
			bool strippable = false;
			for (std::uint32_t slot = 30; slot < 62 && !strippable; ++slot) {
				strippable = (mask & (1u << (slot - 30))) != 0 && Util::IsStrippable(armor, slot);
			}
			if (strippable) {
				worn.push_back(armor);
			} else {
				skip(armor, "body slots only");
			}
		}
		return worn;
	}

	std::vector<MannequinSwap::Piece> MannequinSwap::PlayerPieces(RE::PlayerCharacter* a_player) const
	{
		std::vector<Piece> pieces;
		for (auto* armor : WornOutfit(a_player, nullptr)) {
			pieces.push_back(MakePiece(armor, WornExtra(a_player, armor)));
		}
		for (const auto id : Helmet::GetSingleton()->Stowed()) {
			auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(id);
			if (armor && Util::ItemCount(a_player, armor) > 0 && !IsWearing(a_player, armor) &&
				std::ranges::none_of(pieces, [armor](const Piece& a_piece) { return a_piece.armor == armor; })) {
				pieces.push_back(MakePiece(armor, AnyExtra(a_player, armor)));
			}
		}
		return pieces;
	}

	std::vector<MannequinSwap::Piece> MannequinSwap::MannequinPieces(RE::Actor* a_mannequin) const
	{
		// The same rule as the player's side: mannequins wear the Softbody SMP carrier
		// (HDTSMPObjectBase, non-playable) too, and it must stay on them (first test, 2026-09-26: it
		// counted as the mannequin's outfit and collided with the player's own carrier).
		std::vector<Piece> pieces;
		for (auto* armor : WornOutfit(a_mannequin, nullptr)) {
			pieces.push_back(MakePiece(armor, WornExtra(a_mannequin, armor)));
		}
		return pieces;
	}

	std::string MannequinSwap::Preflight(RE::PlayerCharacter* a_player, const std::vector<Piece>& a_give, const std::vector<Piece>& a_take,
		const Slots& a_slots) const
	{
		if (!a_slots.readable) {
			return "slots unreadable";
		}
		// The mannequin's slots once its own pieces have left.
		auto remaining = a_slots.forms;
		for (const auto& piece : a_take) {
			if (const auto it = std::ranges::find(remaining, piece.armor); it != remaining.end()) {
				*it = nullptr;
			}
		}
		const auto occupied = static_cast<std::size_t>(std::ranges::count_if(remaining, [&](const RE::TESForm* a_form) { return !a_slots.Free(a_form); }));
		const auto free = remaining.size() - occupied;
		if (a_give.size() > free) {
			return std::format("capacity: {} pieces, {} of {} slots free", a_give.size(), free, remaining.size());
		}
		// USSEP refuses a second piece of a base form it already records.
		if (std::string_view(a_slots.variant) == "array") {
			for (std::size_t i = 0; i < a_give.size(); ++i) {
				const auto* armor = a_give[i].armor;
				const bool twice = std::ranges::count_if(a_give, [armor](const Piece& a_piece) { return a_piece.armor == armor; }) > 1;
				if (twice || std::ranges::find(remaining, armor) != remaining.end()) {
					return std::format("duplicate: {} is refused twice by the USSEP script", Util::NameOf(armor));
				}
			}
		}
		// A piece the player keeps on (a device, a shield) must not sit where the mannequin's pieces
		// go; the helmet arrives stowed, so it needs no slot. The SMP carrier is not protected: it
		// sits on slot 60, which modded outfit extras also use (the Eclipse Mage leg plates, USSEP
		// test 2026-09-26), and equipping such a piece by hand displaces it the same way.
		std::uint32_t kept = 0;
		const auto outfit = WornOutfit(a_player, nullptr);
		for (std::uint32_t slot = 30; slot < 62; ++slot) {
			auto* worn = a_player->GetWornArmor(SlotMask(slot));
			if (worn && std::ranges::find(outfit, worn) == outfit.end() && !IsSmpCarrier(worn)) {
				kept |= worn->GetSlotMask().underlying();
			}
		}
		for (const auto& piece : a_take) {
			if (!piece.head && (piece.armor->GetSlotMask().underlying() & kept) != 0) {
				return std::format("kept item: {} would need a slot the player keeps occupied", Util::NameOf(piece.armor));
			}
		}
		return {};
	}

	void MannequinSwap::Tick()
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		if (phase != Phase::kIdle) {
			swap.Update(false, {});
			return;
		}
		std::string how;
		auto* mannequin = FindMannequin(player, how);
		current = mannequin ? static_cast<RE::TESObjectREFR*>(mannequin)->GetHandle() : RE::ObjectRefHandle{};
		if (!mannequin) {
			LogGate("mannequin=-");
			swap.Update(false, {});
			return;
		}
		if (dismissedFor && (dismissedFor != current || player->GetPosition().GetDistance(dismissedAt) > kDismissDistance)) {
			dismissedFor = {};
		}
		const bool dismissed = static_cast<bool>(dismissedFor);
		const bool combat = player->IsInCombat();
		const char* scene = Util::SceneOf(player);
		const bool party = PartyOutfit::GetSingleton()->Active();
		const bool helmetBusy = Helmet::GetSingleton()->Busy();

		const auto mine = PlayerPieces(player);
		const auto theirs = MannequinPieces(mannequin);
		const auto slots = ReadSlots(mannequin);
		const bool anything = !mine.empty() || !theirs.empty();
		const std::string block = anything ? Preflight(player, mine, theirs, slots) : std::string{};

		std::string keptMine;
		std::string keptTheirs;
		WornOutfit(player, &keptMine);
		WornOutfit(mannequin, &keptTheirs);
		LogGate(std::format("mannequin={} ({:08X}) via {} slots={} {}/{} give={} take={} combat={} scene={} party={} helmetBusy={} dismissed={} block={} stays on player:{} stays on mannequin:{}",
			Util::NameOf(mannequin), mannequin->GetFormID(), how, slots.variant, slots.Occupied(), slots.forms.size(), mine.size(), theirs.size(),
			combat, scene ? scene : "-", party, helmetBusy, dismissed, block.empty() ? "-" : block, keptMine.empty() ? " -" : keptMine,
			keptTheirs.empty() ? " -" : keptTheirs));

		if (!block.empty()) {
			const auto key = std::format("{:08X} {}", mannequin->GetFormID(), block.substr(0, block.find(':')));
			if (key != notifiedBlock) {
				notifiedBlock = key;
				Log("WARN no swap with this mannequin: {}", block);
				if (block.starts_with("capacity")) {
					Util::NotifyDiagnostic(Text::L("CIGAR: 마네킹 슬롯 부족. 의상 교환 불가", "CIGAR: The mannequin has too few free slots for this outfit"));
				} else if (block.starts_with("duplicate")) {
					Util::NotifyDiagnostic(Text::L("CIGAR: 마네킹이 같은 장비를 두 개 받지 않음. 의상 교환 불가", "CIGAR: The mannequin refuses a second piece of the same armor"));
				} else if (block.starts_with("kept")) {
					Util::NotifyDiagnostic(Text::L("CIGAR: 벗을 수 없는 장비가 자리를 차지함. 의상 교환 불가", "CIGAR: A piece you cannot take off blocks the mannequin's outfit"));
				} else {
					Util::NotifyDiagnostic(Text::L("CIGAR: 마네킹 슬롯을 읽을 수 없음. 로그 확인", "CIGAR: The mannequin's slots cannot be read. See the log"));
				}
			}
		}

		const bool can = anything && block.empty() && !combat && !scene && !party && !helmetBusy && !dismissed;
		const bool both = !mine.empty() && !theirs.empty();
		const bool store = !mine.empty() && theirs.empty();
		swap.Update(can, [both, store] {
			if (both) {
				return std::string(Text::L("의상 교환 (길게)", "Swap Outfits (hold)"));
			}
			return std::string(store ? Text::L("의상 보관 (길게)", "Store Outfit (hold)") : Text::L("의상 착용 (길게)", "Wear Mannequin's Outfit (hold)"));
		});
	}

	void MannequinSwap::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kSwap || phase != Phase::kIdle) {
			return;
		}
		auto* player = Util::Player();
		std::string how;
		auto* mannequin = player ? FindMannequin(player, how) : nullptr;
		if (!mannequin) {
			Log("accepted, but no mannequin under the crosshair any more");
			return;
		}
		Begin(player, mannequin);
	}

	void MannequinSwap::OnDeclined(std::uint16_t a_eventID)
	{
		if (a_eventID != kSwap || !current) {
			return;
		}
		dismissedFor = current;
		if (auto* player = Util::Player()) {
			dismissedAt = player->GetPosition();
		}
		swap.Withdraw();
		Log("declined: hidden for this mannequin until the player moves {:.0f} units away", kDismissDistance);
	}

	void MannequinSwap::OnDisabled()
	{
		if (phase != Phase::kIdle) {
			Log("WARN switched off in the middle of a swap: the pieces stay where they are now");
		}
		// Begin took Helmet's stowed list; while the take is waiting those helmets are still the player's,
		// so hand them back or Helmet forgets them (review 2026-09-30).
		if (phase == Phase::kTakeWait) {
			for (const auto id : stowedHelmets) {
				Helmet::GetSingleton()->Stow(id);
			}
		}
		Reset();
	}

	void MannequinSwap::Begin(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin)
	{
		give = PlayerPieces(a_player);
		take = MannequinPieces(a_mannequin);
		const auto slots = ReadSlots(a_mannequin);
		if (const auto block = Preflight(a_player, give, take, slots); !block.empty()) {
			Log("WARN swap refused at the press: {}", block);
			Reset();
			return;
		}
		target = static_cast<RE::TESObjectREFR*>(a_mannequin)->GetHandle();
		for (const auto& piece : give) {
			if (piece.head && !IsWearing(a_player, piece.armor)) {
				stowedHelmets.push_back(piece.armor->GetFormID());
			}
		}
		if (!stowedHelmets.empty()) {
			Helmet::GetSingleton()->TakeStowed();
		}
		for (const auto* list : { &give, &take }) {
			for (const auto& piece : *list) {
				countsBefore[piece.armor->GetFormID()] = Util::ItemCount(a_player, piece.armor) + Util::ItemCount(a_mannequin, piece.armor);
				slotCountsBefore[piece.armor->GetFormID()] = slots.CountOf(piece.armor);
			}
		}
		Log("swap with {} ({:08X}): give {}, take {}; slots {} {}/{}", Util::NameOf(a_mannequin), a_mannequin->GetFormID(), give.size(),
			take.size(), slots.variant, slots.Occupied(), slots.forms.size());
		for (const auto& piece : give) {
			Log("  give: {}{}", piece.text, piece.head ? " [head]" : "");
		}
		for (const auto& piece : take) {
			Log("  take: {}{}", piece.text, piece.head ? " [head]" : "");
		}

		// The mannequin's pieces leave first: an incoming piece could otherwise unequip an outgoing one
		// whose slot the script then empties while it is still in the mannequin's inventory.
		for (const auto& piece : take) {
			const auto before = Util::ItemCount(a_player, piece.armor);
			a_mannequin->RemoveItem(piece.armor, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, piece.extra, a_player);
			if (Util::ItemCount(a_player, piece.armor) <= before) {
				Abort(a_player, a_mannequin, std::format("{} did not move to the player", piece.text));
				return;
			}
			taken.push_back(piece);
		}
		phase = Phase::kTakeWait;
		phaseStart = Clock::now();
	}

	void MannequinSwap::Give(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin)
	{
		for (const auto& piece : give) {
			const auto before = Util::ItemCount(a_mannequin, piece.armor);
			// Captured at accept time, seconds ago: the list may be gone (taken off in a menu, a helmet
			// stowed), and a freed list passed to RemoveItem would corrupt the heap. Look it up again.
			auto* extra = HasExtra(a_player, piece.armor, piece.extra) ? piece.extra : WornExtra(a_player, piece.armor);
			if (extra != piece.extra) {
				Log("  give: {}: its item data changed since the accept; using the {}", piece.text, extra ? "worn copy" : "first copy");
			}
			a_player->RemoveItem(piece.armor, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, extra, a_mannequin);
			if (Util::ItemCount(a_mannequin, piece.armor) <= before) {
				Abort(a_player, a_mannequin, std::format("{} did not move to the mannequin", piece.text));
				return;
			}
			given.push_back(piece);
		}
		phase = Phase::kGiveWait;
		phaseStart = Clock::now();
	}

	void MannequinSwap::EquipTaken(RE::PlayerCharacter* a_player)
	{
		auto* manager = RE::ActorEquipManager::GetSingleton();
		for (const auto& piece : taken) {
			if (piece.head) {
				// The user's decision (2026-09-25): the mannequin's helmet arrives stowed, so the face
				// stays visible and 투구 쓰기 offers it in combat.
				Helmet::GetSingleton()->Stow(piece.armor->GetFormID());
				Log("  {} arrives stowed", piece.text);
				continue;
			}
			const bool same = HasExtra(a_player, piece.armor, piece.extra);
			if (manager) {
				manager->EquipObject(a_player, piece.armor, same ? piece.extra : nullptr, 1, nullptr, true, false, false);
			}
			Log("  equip {} ({})", piece.text, same ? "the same instance" : piece.extra ? "WARN its instance was not found; the game picks one" : "plain");
		}
	}

	void MannequinSwap::Finish(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin)
	{
		const auto slots = ReadSlots(a_mannequin);
		std::size_t problems = 0;
		for (const auto& piece : taken) {
			if (!piece.head && !IsWearing(a_player, piece.armor)) {
				++problems;
				Log("WARN player does not wear {}", piece.text);
			}
		}
		for (const auto& piece : given) {
			const bool carried = Util::ItemCount(a_mannequin, piece.armor) > 0;
			const bool worn = IsWearing(a_mannequin, piece.armor);
			const bool recorded = slots.CountOf(piece.armor) > 0;
			if (!carried || !worn || !recorded) {
				++problems;
				Log("WARN mannequin {}: carried={} worn={} recorded={}{}", piece.text, carried, worn, recorded,
					carried ? "" : " (its script sent it back to the player)");
			} else {
				Log("  mannequin wears and records {} (it re-dresses it after a cell load)", piece.text);
			}
		}
		for (const auto& [id, before] : countsBefore) {
			auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(id);
			const auto after = armor ? Util::ItemCount(a_player, armor) + Util::ItemCount(a_mannequin, armor) : -1;
			if (after != before) {
				++problems;
				Log("WARN item count of {:08X} changed: {} before, {} after (player + mannequin)", id, before, after);
			}
		}
		Dress::GetSingleton()->OutfitChanged(a_player);
		Log("swap done: {} given, {} taken, slots {} {}/{}, {} problems", given.size(), taken.size(), slots.variant, slots.Occupied(),
			slots.forms.size(), problems);
		if (problems > 0) {
			Util::Notify(Text::L("CIGAR: 의상 교환 일부 미완료. 로그 확인", "CIGAR: The outfit swap did not fully complete. See the log"));
		}
		Reset();
	}

	void MannequinSwap::Abort(RE::PlayerCharacter* a_player, RE::Actor* a_mannequin, std::string_view a_why)
	{
		Log("WARN swap aborted: {}; moving back what already moved", a_why);
		auto* manager = RE::ActorEquipManager::GetSingleton();
		if (a_player && a_mannequin) {
			for (const auto& piece : given) {
				a_mannequin->RemoveItem(piece.armor, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer,
					HasExtra(a_mannequin, piece.armor, piece.extra) ? piece.extra : nullptr, a_player);
				if (manager && !(piece.head && std::ranges::find(stowedHelmets, piece.armor->GetFormID()) != stowedHelmets.end())) {
					manager->EquipObject(a_player, piece.armor, HasExtra(a_player, piece.armor, piece.extra) ? piece.extra : nullptr, 1, nullptr, true, false, false);
				}
			}
			for (const auto& piece : taken) {
				a_player->RemoveItem(piece.armor, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer,
					HasExtra(a_player, piece.armor, piece.extra) ? piece.extra : nullptr, a_mannequin);
			}
		}
		for (const auto id : stowedHelmets) {
			Helmet::GetSingleton()->Stow(id);
		}
		if (a_player && a_mannequin) {
			Util::Notify(Text::L("CIGAR: 의상 교환 실패. 원래대로 되돌림", "CIGAR: The outfit swap failed and was undone"));
		} else {
			// Nothing was undone: say that, not "undone" (review 2026-09-30).
			Util::Notify(Text::L("CIGAR: 의상 교환 중 마네킹이 사라짐. 로그 확인", "CIGAR: The mannequin vanished during the swap. See the log"));
		}
		Reset();
	}

	void MannequinSwap::FastTick()
	{
		if (phase == Phase::kIdle) {
			return;
		}
		auto* player = Util::Player();
		const auto ref = target.get();
		auto* mannequin = ref ? ref->As<RE::Actor>() : nullptr;
		if (!player || !mannequin) {
			Abort(player, mannequin, "the mannequin is gone");
			return;
		}
		const auto elapsed = Clock::now() - phaseStart;
		if (phase == Phase::kTakeWait) {
			// Not only unequipped: the script must have emptied the slot, or USSEP would refuse an
			// incoming piece of the same base form as a duplicate.
			const auto slots = ReadSlots(mannequin);
			const bool released = std::ranges::all_of(taken, [&](const Piece& a_piece) {
				const auto id = a_piece.armor->GetFormID();
				return !IsWearing(mannequin, a_piece.armor) && slots.CountOf(a_piece.armor) < std::max<std::size_t>(slotCountsBefore[id], 1);
			});
			if (released || elapsed > kScriptWait) {
				Log("mannequin let go of its pieces after {} ms{}", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
					released ? "" : " (WARN not all slots emptied; continuing)");
				Give(player, mannequin);
			}
		} else if (phase == Phase::kGiveWait) {
			const auto slots = ReadSlots(mannequin);
			const bool done = std::ranges::all_of(given, [&](const Piece& a_piece) {
				return IsWearing(mannequin, a_piece.armor) && slots.CountOf(a_piece.armor) > 0;
			});
			if (done || elapsed > kScriptWait) {
				Log("mannequin took the player's pieces after {} ms{}", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(),
					done ? "" : " (not all worn and recorded yet; the final check tells)");
				EquipTaken(player);
				phase = Phase::kSettle;
				phaseStart = Clock::now();
			}
		} else if (phase == Phase::kSettle && elapsed > kSettleWait) {
			Finish(player, mannequin);
		}
	}
}
