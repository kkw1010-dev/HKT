#include "Execute.h"

#include "Settings.h"
#include "Util.h"

#include "ValhallaCombat/API.h"

namespace CIGAR
{
	namespace
	{
		// Valhalla Combat 1.3.3 reads only this file (at kDataLoaded, and again when its MCM closes).
		// A key missing from it takes the DLL's compiled default: iExecutionKey -1, bStunToggle 1.
		constexpr auto kValhallaSettings = "Data/MCM/Settings/ValhallaCombat.ini"sv;
		constexpr auto kRaceMappingDir = "Data/SKSE/Plugins/ValhallaCombat/RaceMapping"sv;
		constexpr auto kValhallaPlugin = "ValhallaCombat.esp"sv;
		constexpr RE::FormID kMCMQuestID = 0xD62;  // ValhallaCombat_MCM_Quest
		constexpr auto kMCMScript = "valhallaCombat_MCM";
		constexpr auto kPromptOnlyTarget = "valhalla"sv;

		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;

		// executionHandler::tryPcExecution: the nearest stun-broken actor strictly within 250 units.
		constexpr float kReach = 250.0f;
		// Valhalla queues the kill-move idle from a thread in 50 ms steps; allow for a slow frame.
		constexpr auto kKillMoveWindow = 2s;
		// Utils::Actor::isHumanoid: the race's body part data is DefaultBodyPartData.
		constexpr RE::FormID kHumanoidBodyPartData = 0x1D;

		constexpr std::array kCategoryNames{
			"Humanoid"sv, "Undead"sv, "Falmer"sv, "Spider"sv, "Gargoyle"sv, "Giant"sv, "Bear"sv,
			"SabreCat"sv, "Wolf"sv, "Troll"sv, "Hagraven"sv, "Spriggan"sv, "Boar"sv, "Riekling"sv,
			"AshHopper"sv, "SteamCenturion"sv, "DwarvenBallista"sv, "ChaurusFlyer"sv, "Lurker"sv, "Dragon"sv
		};

		std::string_view Trim(std::string_view a_text)
		{
			const auto first = a_text.find_first_not_of(" \t\r");
			if (first == std::string_view::npos) {
				return {};
			}
			return a_text.substr(first, a_text.find_last_not_of(" \t\r") - first + 1);
		}

		bool IsHumanoid(RE::Actor* a_actor)
		{
			const auto* race = a_actor ? a_actor->GetRace() : nullptr;
			return race && race->bodyPartData && race->bodyPartData->GetFormID() == kHumanoidBodyPartData;
		}

		// Utils::Actor::getWieldingWeapon: the attacking weapon, else the right hand, else the left.
		RE::TESObjectWEAP* WieldingWeapon(RE::Actor* a_actor)
		{
			if (const auto* attacking = a_actor->GetAttackingWeapon(); attacking && attacking->object) {
				if (auto* weapon = attacking->object->As<RE::TESObjectWEAP>()) {
					return weapon;
				}
			}
			for (const bool left : { false, true }) {
				auto* form = a_actor->GetEquippedObject(left);
				if (form && form->IsWeapon()) {
					return form->As<RE::TESObjectWEAP>();
				}
			}
			return nullptr;
		}

		// Utils::Actor::isDualWielding: a weapon in each hand, the right one not two-handed.
		bool DualWielding(RE::Actor* a_actor)
		{
			auto* left = a_actor->GetEquippedObject(true);
			auto* right = a_actor->GetEquippedObject(false);
			if (!left || !right || !left->IsWeapon() || !right->IsWeapon()) {
				return false;
			}
			const auto type = right->As<RE::TESObjectWEAP>()->GetWeaponType();
			return type != RE::WEAPON_TYPE::kTwoHandAxe && type != RE::WEAPON_TYPE::kTwoHandSword;
		}

		class ReloadResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable) override
			{
				Execute::GetSingleton()->Log("Valhalla settings reload (OnConfigClose) returned");
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};
	}

	Execute::Execute()
	{
		// A failed press (see the kill-move check) should not leave the prompt gone while it still holds.
		execute.SetRepeat(true);
	}

	Execute* Execute::GetSingleton()
	{
		static Execute singleton;
		return &singleton;
	}

	void Execute::PrepareKey()
	{
		if (!GetModuleHandleW(L"ValhallaCombat.dll")) {
			Log("Valhalla Combat not loaded; its execution key is left alone");
			return;
		}
		ApplyKeyMode();
	}

	bool Execute::ApplyKeyMode()
	{
		const std::filesystem::path path{ kValhallaSettings };
		const auto current = Util::IniInt(path, "Stun", "iExecutionKey").value_or(-1);
		stunEnabled = Util::IniInt(path, "Stun", "bStunToggle").value_or(1) != 0;
		const bool promptOnly = Settings::PromptOnly(kPromptOnlyTarget);
		auto wanted = current;
		if (promptOnly) {
			if (current != kHiddenKey) {
				// Remember the player's key so switching prompt-only off gives it back.
				if (current >= 0 && current < 264) {
					Settings::SetManualKey(kPromptOnlyTarget, static_cast<std::int32_t>(current));
				}
				wanted = kHiddenKey;
			}
		} else if (current == kHiddenKey) {
			wanted = Settings::ManualKey(kPromptOnlyTarget);
		}
		executionKey = wanted;
		if (wanted == current) {
			Log("Valhalla iExecutionKey {} (prompt-only={}, stun={})", current, promptOnly, stunEnabled);
			return false;
		}
		const bool written = Util::IniSetInt(path, "Stun", "iExecutionKey", wanted);
		Log("{}Valhalla iExecutionKey {} -> {} (prompt-only={}, stun={}) written={}", written ? "" : "WARN ", current, wanted,
			promptOnly, stunEnabled, written);
		return written;
	}

	void Execute::ReloadValhalla()
	{
		auto* handler = RE::TESDataHandler::GetSingleton();
		auto* quest = handler ? handler->LookupForm<RE::TESQuest>(kMCMQuestID, kValhallaPlugin) : nullptr;
		if (!quest || !Util::ScriptObject(quest, kMCMScript)) {
			Log("WARN Valhalla MCM quest/script not found; the key applies after a restart");
			return;
		}
		// valhallaCombat_MCM.OnConfigClose is Valhalla's native that re-reads its settings file.
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		const auto handle = vm->GetObjectHandlePolicy()->GetHandleForObject(quest->GetFormType(), quest);
		auto* args = RE::MakeFunctionArguments();
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new ReloadResult() };
		const bool queued = vm->DispatchMethodCall2(handle, kMCMScript, "OnConfigClose", args, callback);
		Log("Valhalla settings reload requested queued={}", queued);
	}

	void Execute::CheckKey()
	{
		if (!present.load()) {
			Log("key check: Valhalla Combat not loaded");
			return;
		}
		// A key set in Valhalla's MCM lands in its INI; with prompt-only on it is remembered and moved back.
		if (ApplyKeyMode()) {
			ReloadValhalla();
		}
		const auto key = executionKey.load();
		Log("key check: Valhalla execution key {} (usable={})", key, key >= 0 && key < 264);
	}

	void Execute::LoadRaceMap()
	{
		races.clear();
		auto* handler = RE::TESDataHandler::GetSingleton();
		std::error_code ec;
		std::size_t files = 0;
		// data::loadExecutableRace: every .ini in the folder; a race keeps the first category it gets.
		for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path{ kRaceMappingDir }, ec)) {
			std::ifstream file{ entry.path() };
			if (!file) {
				continue;
			}
			++files;
			std::string line;
			std::optional<Category> category;
			while (std::getline(file, line)) {
				auto text = Trim(line);
				if (text.starts_with("\xEF\xBB\xBF"sv)) {
					text.remove_prefix(3);
				}
				if (text.empty() || text.front() == ';' || text.front() == '#') {
					continue;
				}
				if (text.front() == '[') {
					category.reset();
					const auto name = text.substr(1, text.size() > 2 ? text.size() - 2 : 0);
					for (std::size_t i = 0; i < kCategoryNames.size(); ++i) {
						if (Util::EqualsNoCase(name, kCategoryNames[i])) {
							category = static_cast<Category>(i);
						}
					}
					continue;
				}
				const auto eq = text.find('=');
				const auto bar = text.find('|');
				if (!category || eq == std::string_view::npos || bar == std::string_view::npos || bar < eq) {
					continue;
				}
				const auto plugin = Trim(text.substr(eq + 1, bar - eq - 1));
				auto id = Trim(text.substr(bar + 1));
				int base = 10;
				if (id.size() > 2 && id[0] == '0' && (id[1] == 'x' || id[1] == 'X')) {
					id.remove_prefix(2);
					base = 16;
				}
				RE::FormID formID = 0;
				if (std::from_chars(id.data(), id.data() + id.size(), formID, base).ec != std::errc{}) {
					continue;
				}
				if (auto* race = handler->LookupForm<RE::TESRace>(formID, plugin)) {
					races.emplace(race, *category);
				}
			}
		}
		Log("race map: {} races from {} files in {}", races.size(), files, kRaceMappingDir);
	}

	void Execute::OnGameLoaded()
	{
		execute.Reset();
		lastGate.clear();
		offeredVictim = nullptr;
		pressPending = false;
		present = false;

		if (!GetModuleHandleW(L"ValhallaCombat.dll")) {
			Log("Valhalla Combat not found; the execution prompt is off");
			return;
		}
		if (!api) {
			api = VAL_API::RequestPluginAPI();
		}
		auto* handler = RE::TESDataHandler::GetSingleton();
		sexlabAnimating = handler && handler->LookupModByName(kSexLabPlugin) ?
		                      handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin) :
		                      nullptr;
		LoadRaceMap();
		Log("Valhalla api={} races={} sexlab={}", api != nullptr, races.size(), sexlabAnimating != nullptr);
		if (!api || races.empty()) {
			Log("WARN Valhalla Combat is loaded but {}; the execution prompt is off", !api ? "its API (V2) was not returned" : "no race mapping was read");
			if (!warned) {
				warned = true;
				Util::Notify("CIGAR: Valhalla 연동 실패. 처형 프롬프트 비활성");
			}
			return;
		}
		present = true;
		CheckKey();
		if (!stunEnabled) {
			Log("WARN Valhalla stun is off (bStunToggle=0); nothing can be executed");
		}
		Log("ready");
	}

	RE::Actor* Execute::FindVictim(RE::PlayerCharacter* a_player, float& a_distance) const
	{
		RE::Actor* victim = nullptr;
		a_distance = kReach;
		auto* lists = RE::ProcessLists::GetSingleton();
		if (!lists) {
			return nullptr;
		}
		const auto origin = a_player->GetPosition();
		for (auto& handle : lists->highActorHandles) {
			auto* actor = handle.get().get();
			if (!actor || actor == a_player || !actor->Is3DLoaded() || actor->IsDead() || actor->IsInKillMove()) {
				continue;
			}
			const float distance = origin.GetDistance(actor->GetPosition());
			// Valhalla's own loop keeps an actor only when it is strictly nearer than 250.
			if (distance >= a_distance || !api->isActorStunned(actor)) {
				continue;
			}
			victim = actor;
			a_distance = distance;
		}
		return victim;
	}

	bool Execute::Executable(RE::PlayerCharacter* a_player, std::string& a_gate, RE::Actor*& a_victim) const
	{
		a_victim = nullptr;
		const auto* controls = RE::ControlMap::GetSingleton();
		const auto key = executionKey.load();
		std::string why;
		float distance = 0.0f;
		if (!stunEnabled) {
			why = "stun-off";
		} else if (key < 0 || key >= 264) {
			why = "no-key";
		} else if (!controls || !controls->IsMovementControlsEnabled()) {
			why = "no-controls";
		} else if (sexlabAnimating && a_player->IsInFaction(sexlabAnimating)) {
			why = "sexlab";
		} else if (a_player->IsDead() || a_player->IsInKillMove() || a_player->IsOnMount() || !IsHumanoid(a_player)) {
			why = "player";  // executionHandler::attemptExecute's executor checks
		} else if (a_victim = FindVictim(a_player, distance); !a_victim) {
			why = "no-stunned-target";
		}
		std::string victimName = a_victim ? Util::NameOf(a_victim) : "-"s;
		if (why.empty()) {
			// executionHandler::attemptExecute's victim checks, in its order.
			auto* weapon = WieldingWeapon(a_player);
			const auto type = weapon ? weapon->GetWeaponType() : RE::WEAPON_TYPE::kHandToHandMelee;
			const auto race = races.find(a_victim->GetRace());
			if (a_victim->IsOnMount()) {
				why = "target-mounted";
			} else if (a_victim->IsPlayerTeammate()) {
				why = "target-teammate";
			} else if (a_victim->IsEssential()) {
				why = "target-essential";
			} else if (auto* magic = a_victim->AsMagicTarget(); magic && magic->HasEffectWithArchetype(RE::MagicTarget::Archetype::kParalysis)) {
				why = "target-paralysed";
			} else if (race == races.end()) {
				why = "race-unmapped";
			} else if (type == RE::WEAPON_TYPE::kBow || type == RE::WEAPON_TYPE::kCrossbow) {
				why = "ranged-weapon";
			} else if (race->second == Category::kHumanoid) {
				// executeHumanoid: dual wield and back attacks cover every weapon; from the front a
				// staff has no kill move.
				const float angle = a_victim->GetHeadingAngle(a_player->GetPosition(), false);
				const bool back = angle > 90.0f || angle < -90.0f;
				if (!DualWielding(a_player) && !back && type == RE::WEAPON_TYPE::kStaff) {
					why = "staff-front";
				}
			} else if (type == RE::WEAPON_TYPE::kHandToHandMelee) {
				// Every other category has no unarmed kill move.
				why = std::format("unarmed-vs-{}", kCategoryNames[static_cast<std::size_t>(race->second)]);
			}
		}
		const bool ok = why.empty();
		a_gate = std::format("target={} ok={} why={} key={} stun={}", victimName, ok, ok ? "-"s : why, key, stunEnabled);
		if (!ok) {
			a_victim = nullptr;
		}
		return ok;
	}

	void Execute::FastTick()
	{
		if (!present.load()) {
			return;
		}
		auto* player = Util::Player();
		if (pressPending) {
			auto victim = pressedVictim.get();
			const bool started = player->IsInKillMove() || (victim && victim->IsInKillMove());
			if (started) {
				pressPending = false;
				Log("kill move started on {}", victim ? Util::NameOf(victim.get()) : "-"s);
			} else if (Clock::now() >= pressDeadline) {
				pressPending = false;
				// Known cause: tryPcExecution stops its whole search at the first stale stun-broken entry
				// (dead, unloaded, in a kill move or out of high process) instead of skipping it.
				Log("WARN execution key {} pressed but no kill move started within 2 s (target {}); Valhalla may have hit a stale stunned actor",
					executionKey.load(), victim ? Util::NameOf(victim.get()) : "-"s);
				if (!warnedNoKillMove) {
					warnedNoKillMove = true;
					Util::Notify("CIGAR: 처형 키 입력 후 처형 미발동. 로그 확인");
				}
			}
		}
		std::string gate;
		RE::Actor* victim = nullptr;
		const bool live = !pressPending && Executable(player, gate, victim);
		LogGate(std::move(gate));
		// The prompt names the target; when it changes, offer it again with the new name.
		if (execute.Offered() && victim != offeredVictim) {
			execute.Withdraw();
			execute.Reset();
		}
		offeredVictim = live ? victim : nullptr;
		execute.Update(live, [victim] { return std::format("처형: {}", Util::NameOf(victim)); });
	}

	void Execute::OnAccepted(std::uint16_t a_eventID)
	{
		if (!present.load() || a_eventID != kExecute || pressPending) {
			return;
		}
		auto* player = Util::Player();
		std::string gate;
		RE::Actor* victim = nullptr;
		// Re-check: the stun may have run out while the prompt was up.
		if (!Executable(player, gate, victim)) {
			Log("accept ignored, no longer executable: {}", gate);
			return;
		}
		const auto key = executionKey.load();
		const bool pressed = Util::PressKey(key);
		Log("execute {} ({:08X}) key {} pressed={}", Util::NameOf(victim), victim->GetFormID(), key, pressed);
		if (pressed) {
			pressPending = true;
			pressedVictim = victim->GetHandle();
			pressDeadline = Clock::now() + kKillMoveWindow;
		}
	}
}
