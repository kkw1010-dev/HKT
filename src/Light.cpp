#include "Light.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kTCLPlugin = "TorchesCandlelightLanterns.esp"sv;
		// i329TCL_MCM, which carries i329TCL_MCMConfig_Script (an MCM Helper MCM_ConfigBase). Its
		// OnSettingChange re-reads iTCLHotkey:Controls and re-registers the key (read from the
		// script's string table, 2026-09-24).
		constexpr RE::FormID kMCMQuestID = 0x89F;
		constexpr auto kMCMScript = "i329TCL_MCMConfig_Script"sv;
		constexpr auto kHotkeySetting = "iTCLHotkey:Controls"sv;
		// i329Hotkey: the key TCL registered, which OnKeyUp compares against.
		constexpr RE::FormID kHotkeyGlobalID = 0x8E7;
		constexpr RE::FormID kLanternHandOnID = 0x86F;
		// Lit lanterns: i329ListHand, i329ListHandSMP, i329ListHandSMPRotate, i329ListHip. The unlit
		// ones are separate armors ("...Off") in other lists, with no keyword to tell them apart.
		constexpr std::array kLitListIDs{ RE::FormID{ 0x865 }, RE::FormID{ 0x86D }, RE::FormID{ 0x86E }, RE::FormID{ 0x86A } };
		constexpr RE::FormID kCandlelightKeywordID = 0x931;  // i329IsCandlelightSpell
		// MCM Helper's user file wins over the mod's default file.
		constexpr auto kUserIni = "Data/MCM/Settings/TorchesCandlelightLanterns.ini"sv;
		constexpr auto kDefaultIni = "Data/MCM/Config/TorchesCandlelightLanterns/settings.ini"sv;

		// Prompt-only moves TCL's key here: a scan code no keyboard sends. F13-F15 (0x64-0x66) are
		// Grapple's, Surrender's and Valhalla's.
		constexpr std::int32_t kHiddenKey = 0x67;
		constexpr auto kPromptOnlyTarget = "tcl"sv;

		// SI's ItemUse values: darkness_threshold 14, time_till_makelight_prompt 5 s,
		// dont_show_in_combat_makelight on.
		constexpr float kDarkLevel = 14.0f;
		constexpr auto kDarkDelay = 5s;
		// After the press, TCL (Papyrus) gets this long to light something before the log says it did not.
		constexpr auto kLitCheck = 3s;

		// The engine's own GetLightLevel condition on the player, so the number means what the
		// game's conditions mean. CommonLib has no direct accessor.
		bool LightBelow(RE::PlayerCharacter* a_player, float a_level)
		{
			RE::TESConditionItem item;
			item.data.object = RE::CONDITIONITEMOBJECT::kSelf;
			item.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetLightLevel;
			item.data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kLessThan;
			item.data.comparisonValue.f = a_level;
			RE::ConditionCheckParams params(a_player, nullptr);
			return item.IsTrue(params);
		}

		class ResultThen final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			explicit ResultThen(std::function<void()> a_then) :
				then(std::move(a_then)) {}

			void operator()(RE::BSScript::Variable) override
			{
				if (then) {
					SKSE::GetTaskInterface()->AddTask(then);
				}
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		private:
			std::function<void()> then;
		};
	}

	Light::Light()
	{
		prompt.SetPromptType(SkyPromptAPI::kHold);
	}

	Light* Light::GetSingleton()
	{
		static Light singleton;
		return &singleton;
	}

	void Light::OnGameLoaded()
	{
		prompt.Reset();
		lastGate.clear();
		darkSince = Clock::now();
		checkAfterAccept = false;
		mcmQuest = nullptr;
		hotkeyGlobal = nullptr;
		lanternHandOn = nullptr;
		litLanterns.clear();
		candlelightKeyword = nullptr;
		tclKey = -1;

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kTCLPlugin)) {
			Log("{} not loaded: 불 밝히기 is off", kTCLPlugin);
			return;
		}
		mcmQuest = handler->LookupForm<RE::TESQuest>(kMCMQuestID, kTCLPlugin);
		hotkeyGlobal = handler->LookupForm<RE::TESGlobal>(kHotkeyGlobalID, kTCLPlugin);
		lanternHandOn = handler->LookupForm<RE::TESGlobal>(kLanternHandOnID, kTCLPlugin);
		candlelightKeyword = handler->LookupForm<RE::BGSKeyword>(kCandlelightKeywordID, kTCLPlugin);
		for (const auto id : kLitListIDs) {
			if (auto* list = handler->LookupForm<RE::BGSListForm>(id, kTCLPlugin)) {
				litLanterns.push_back(list);
			}
		}
		const bool script = mcmQuest && Util::ScriptObject(mcmQuest, kMCMScript.data());
		Log("TCL: quest={} script={} hotkey global={} lit lantern lists={}/{} candlelight keyword={}",
			mcmQuest != nullptr, script, hotkeyGlobal != nullptr, litLanterns.size(), kLitListIDs.size(),
			candlelightKeyword != nullptr);
		if (!mcmQuest || !script || !hotkeyGlobal) {
			Log("WARN TCL forms or script missing: 불 밝히기 is off");
			Util::Notify("CIGAR: TCL 연동 실패. 로그 확인");
			mcmQuest = nullptr;
			return;
		}
		Util::WarnIfSIModuleOn("ItemUse.enabled_makelight", "/MCP/modules/ItemUse/enabled_makelight");
		ApplyKeyMode();
	}

	void Light::Tick()
	{
		Util::WarnIfSIModuleOn("ItemUse.enabled_makelight", "/MCP/modules/ItemUse/enabled_makelight");
		if (hotkeyGlobal) {
			tclKey = static_cast<std::int32_t>(hotkeyGlobal->value);
		}
	}

	void Light::ApplyKeyMode()
	{
		if (!mcmQuest) {
			return;
		}
		auto current = Util::IniInt(std::filesystem::path{ kUserIni }, "Controls", "iTCLHotkey");
		if (!current) {
			current = Util::IniInt(std::filesystem::path{ kDefaultIni }, "Controls", "iTCLHotkey");
		}
		const auto have = static_cast<std::int32_t>(current.value_or(-1));
		const bool promptOnly = Settings::PromptOnly(kPromptOnlyTarget);
		auto wanted = have;
		if (promptOnly) {
			if (have != kHiddenKey) {
				// Remember the player's key so switching prompt-only off gives it back.
				if (have >= 0 && have < 264) {
					Settings::SetManualKey(kPromptOnlyTarget, have);
				}
				wanted = kHiddenKey;
			}
		} else if (have == kHiddenKey) {
			wanted = Settings::ManualKey(kPromptOnlyTarget);
		}
		const auto registered = hotkeyGlobal ? static_cast<std::int32_t>(hotkeyGlobal->value) : -1;
		Log("TCL key: ini {} registered {} -> wanted {} (prompt-only={})", have, registered, wanted, promptOnly);
		if (wanted >= 0 && (wanted != have || wanted != registered)) {
			SetTCLKey(wanted);
		}
	}

	void Light::SetTCLKey(std::int32_t a_key)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm || !mcmQuest) {
			return;
		}
		const auto handle = Util::Handle(mcmQuest);
		// MCM Helper stores the value (and writes its user INI); OnSettingChange makes TCL re-read it
		// and re-register the key, so no reload is needed. Chained so the second call sees the first.
		auto* setArgs = RE::MakeFunctionArguments(RE::BSFixedString{ kHotkeySetting }, static_cast<std::int32_t>(a_key));
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> afterSet{ new ResultThen([handle, a_key] {
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			auto* changeArgs = RE::MakeFunctionArguments(RE::BSFixedString{ kHotkeySetting });
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> afterChange{ new ResultThen([a_key] {
				auto* self = Light::GetSingleton();
				const auto registered = self->hotkeyGlobal ? static_cast<std::int32_t>(self->hotkeyGlobal->value) : -1;
				self->tclKey = registered;
				self->Log("{}TCL key set to {}: TCL registered {}", registered == a_key ? "" : "WARN ", a_key, registered);
				if (registered != a_key) {
					Util::Notify("CIGAR: TCL 단축키 변경 실패. 로그 확인");
				}
			}) };
			const bool queued = vm && vm->DispatchMethodCall2(handle, kMCMScript, "OnSettingChange", changeArgs, afterChange);
			Light::GetSingleton()->Log("TCL OnSettingChange({}) queued={}", kHotkeySetting, queued);
		}) };
		const bool queued = vm->DispatchMethodCall2(handle, kMCMScript, "SetModSettingInt", setArgs, afterSet);
		Log("TCL SetModSettingInt({}, {}) queued={}", kHotkeySetting, a_key, queued);
	}

	bool Light::Dark(RE::PlayerCharacter* a_player) const
	{
		return LightBelow(a_player, kDarkLevel);
	}

	std::string Light::LightBand(RE::PlayerCharacter* a_player) const
	{
		constexpr std::array kBands{ 5.0f, 14.0f, 30.0f, 60.0f, 100.0f };
		for (const float band : kBands) {
			if (LightBelow(a_player, band)) {
				return std::format("<{:.0f}", band);
			}
		}
		return ">=100";
	}

	bool Light::Lit(RE::PlayerCharacter* a_player, std::string& a_how) const
	{
		for (const bool left : { true, false }) {
			if (auto* object = a_player->GetEquippedObject(left); object && object->Is(RE::FormType::Light)) {
				a_how = "light in hand";
				return true;
			}
		}
		if (auto* effects = a_player->AsMagicTarget()->GetActiveEffectList()) {
			for (auto* effect : *effects) {
				const auto* base = effect ? effect->GetBaseObject() : nullptr;
				if (!base || effect->flags.any(RE::ActiveEffect::Flag::kInactive, RE::ActiveEffect::Flag::kDispelled)) {
					continue;
				}
				if (base->GetArchetype() == RE::EffectArchetypes::ArchetypeID::kLight ||
					(candlelightKeyword && base->HasKeyword(candlelightKeyword))) {
					a_how = "light effect";
					return true;
				}
			}
		}
		if (lanternHandOn && lanternHandOn->value >= 1.0f) {
			a_how = "TCL hand lantern on";
			return true;
		}
		for (auto* list : litLanterns) {
			bool worn = false;
			list->ForEachForm([&](RE::TESForm* a_form) {
				auto* armor = a_form ? a_form->As<RE::TESObjectARMO>() : nullptr;
				worn = armor && a_player->GetWornArmor(armor->GetFormID()) != nullptr;
				return worn ? RE::BSContainer::ForEachResult::kStop : RE::BSContainer::ForEachResult::kContinue;
			});
			if (worn) {
				a_how = "lit TCL lantern worn";
				return true;
			}
		}
		return false;
	}

	void Light::FastTick()
	{
		auto* player = Util::Player();
		if (!player || !mcmQuest) {
			prompt.Update(false, {});
			return;
		}
		const auto now = Clock::now();
		auto* ui = RE::UI::GetSingleton();
		std::string how;
		const bool dark = Dark(player);
		const bool lit = Lit(player, how);
		const bool combat = player->IsInCombat();
		const bool menu = !ui || ui->IsApplicationMenuOpen() || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME);
		const bool keyKnown = tclKey.load() >= 0;
		const bool can = dark && !lit && !combat && !menu && keyKnown && !player->IsDead();
		if (!can) {
			darkSince = now;
		}
		const bool available = can && now - darkSince >= kDarkDelay;

		LogGate(std::format("dark={} light{} lit={}{} combat={} menu={} tclKey={} ready={}", dark, LightBand(player), lit,
			lit ? " (" + how + ")" : "", combat, menu, tclKey.load(), available));

		if (checkAfterAccept && now - acceptedAt >= kLitCheck) {
			checkAfterAccept = false;
			if (lit) {
				Log("lit after the press ({})", how);
			} else {
				Log("still not lit {}s after the press: TCL found nothing to light (no lantern, torch or Candlelight?) or ignored the key",
					std::chrono::duration_cast<std::chrono::seconds>(kLitCheck).count());
			}
		}
		prompt.Update(available, [] { return "불 밝히기 (길게)"s; });
	}

	void Light::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kLight) {
			return;
		}
		const auto key = hotkeyGlobal ? static_cast<std::int32_t>(hotkeyGlobal->value) : tclKey.load();
		const bool pressed = key >= 0 && Util::PressKey(key);
		Log("불 밝히기: pressed TCL key {} ok={}", key, pressed);
		if (!pressed) {
			Util::Notify("CIGAR: TCL 단축키 입력 실패. 로그 확인");
			return;
		}
		acceptedAt = Clock::now();
		checkAfterAccept = true;
	}
}
