#include "Helmet.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kPlugin = "Helmet Toggle 2.esp"sv;
		constexpr auto kMCMScript = "HT_MCM";
		// Helmet Toggle 2.esp: HT_AnimationQuest (holds HT_MCM), and the globals and lists its
		// scripts read (HT_MCM.psc / HT_PlayerAlias.psc).
		constexpr RE::FormID kQuestID = 0x800;
		constexpr RE::FormID kHotkeyTypeID = 0x818;       // 0 off, 1 simple, 2 dynamic
		constexpr RE::FormID kHelmetStateID = 0x804;      // 0 headgear on, >0 hidden
		constexpr RE::FormID kEnableLocationID = 0x80E;
		constexpr RE::FormID kEnableParentID = 0x82A;
		constexpr RE::FormID kEnableInteriorsID = 0x84C;
		constexpr RE::FormID kEnableWeatherID = 0x805;
		constexpr RE::FormID kEnableSeasonsID = 0x851;
		constexpr RE::FormID kSafeListID = 0x801;
		constexpr RE::FormID kUnsafeListID = 0x829;
		constexpr RE::FormID kHoodID = 0x817;
		constexpr RE::FormID kMaskID = 0x82D;
		constexpr RE::FormID kHelmetAltID = 0x87B;
		constexpr RE::FormID kIgnoreID = 0x82B;
		// Skyrim.esm: ArmorHelmet, ClothingHead, ClothingCirclet.
		constexpr RE::FormID kArmorHelmetID = 0x06C0EE;
		constexpr RE::FormID kClothingHeadID = 0x10CD11;
		constexpr RE::FormID kClothingCircletID = 0x10CD08;
		constexpr auto kCheckDelay = 3s;

		class PressResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable) override
			{
				Helmet::GetSingleton()->Log("HT_MCM.PressHotkey returned");
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};

		bool HasAnyKeyword(const RE::BGSKeywordForm* a_form, const RE::BGSListForm* a_list)
		{
			if (!a_form || !a_list) {
				return false;
			}
			for (std::uint32_t i = 0; i < a_form->numKeywords; ++i) {
				if (a_form->keywords[i] && a_list->HasForm(a_form->keywords[i])) {
					return true;
				}
			}
			return false;
		}
	}

	Helmet::Helmet()
	{
		off.SetPromptType(SkyPromptAPI::kHold);
		on.SetPromptType(SkyPromptAPI::kHold);
	}

	Helmet* Helmet::GetSingleton()
	{
		static Helmet singleton;
		return &singleton;
	}

	float Helmet::Value(const RE::TESGlobal* a_global)
	{
		return a_global ? a_global->value : 0.0f;
	}

	void Helmet::OnGameLoaded()
	{
		off.Reset();
		on.Reset();
		lastGate.clear();
		ready = false;
		dismissed = false;
		pending = false;
		location = 0;
		warnedType = false;
		headgearKeywords.clear();

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kPlugin)) {
			Log("{} not loaded: idle", kPlugin);
			return;
		}
		const auto form = [handler](RE::FormID a_id) { return handler->LookupForm(a_id, kPlugin); };
		quest = form(kQuestID) ? form(kQuestID)->As<RE::TESQuest>() : nullptr;
		const auto global = [&form](RE::FormID a_id) {
			auto* f = form(a_id);
			return f ? f->As<RE::TESGlobal>() : nullptr;
		};
		hotkeyType = global(kHotkeyTypeID);
		helmetState = global(kHelmetStateID);
		enableLocation = global(kEnableLocationID);
		enableParent = global(kEnableParentID);
		enableInteriors = global(kEnableInteriorsID);
		enableWeather = global(kEnableWeatherID);
		enableSeasons = global(kEnableSeasonsID);
		safeList = form(kSafeListID) ? form(kSafeListID)->As<RE::BGSListForm>() : nullptr;
		unsafeList = form(kUnsafeListID) ? form(kUnsafeListID)->As<RE::BGSListForm>() : nullptr;
		for (const auto id : { kHoodID, kMaskID, kHelmetAltID }) {
			if (auto* f = form(id); f && f->As<RE::BGSKeyword>()) {
				headgearKeywords.push_back(f->As<RE::BGSKeyword>());
			}
		}
		for (const auto id : { kArmorHelmetID, kClothingHeadID }) {
			if (auto* k = RE::TESForm::LookupByID<RE::BGSKeyword>(id)) {
				headgearKeywords.push_back(k);
			}
		}
		ignoreKeyword = form(kIgnoreID) ? form(kIgnoreID)->As<RE::BGSKeyword>() : nullptr;
		circletKeyword = RE::TESForm::LookupByID<RE::BGSKeyword>(kClothingCircletID);

		const bool script = quest && Util::ScriptObject(quest, kMCMScript);
		ready = script && hotkeyType && helmetState && enableLocation && safeList && unsafeList;
		Util::WarnIfSIModuleOn("HelmetToggle.enabled", "/MCP/modules/HelmetToggle/enabled");
		Log("ready={}: quest={} HT_MCM={} hotkeyType={} helmetState={} location={} parent={} interiorsOnly={} "
			"safe list {} forms, unsafe list {} forms, headgear keywords {}",
			ready, quest != nullptr, script, Value(hotkeyType), Value(helmetState), Value(enableLocation),
			Value(enableParent), Value(enableInteriors), safeList ? safeList->forms.size() : 0,
			unsafeList ? unsafeList->forms.size() : 0, headgearKeywords.size());
		if (Value(enableWeather) != 0.0f || Value(enableSeasons) != 0.0f) {
			Log("note: Helmet Toggle's weather/season options are on; CIGAR reads locations only");
		}
		if (!ready) {
			Log("WARN Helmet Toggle 2 is loaded but a form or HT_MCM is missing: no helmet prompt");
			Util::Notify("CIGAR: Helmet Toggle 연동 실패. 로그 확인");
		}
	}

	bool Helmet::Safe(RE::PlayerCharacter* a_player, std::string& a_why) const
	{
		// HT_PlayerAlias.CheckConditions: with the location option off every place counts as safe.
		if (Value(enableLocation) == 0.0f) {
			a_why = "location option off";
			return true;
		}
		auto* loc = a_player->GetCurrentLocation();
		if (!loc) {
			a_why = "no location";
			return false;
		}
		const auto* cell = a_player->GetParentCell();
		const bool interior = cell && cell->IsInteriorCell();
		if (!interior && Value(enableInteriors) != 0.0f) {
			a_why = "exterior (interiors only)";
			return false;
		}
		// FindLocationKeyword: "a location can have safe and unsafe keywords at the same time".
		if (HasAnyKeyword(loc, unsafeList)) {
			a_why = "unsafe keyword";
			return false;
		}
		if (HasAnyKeyword(loc, safeList)) {
			a_why = "safe keyword";
			return true;
		}
		if (Value(enableParent) != 0.0f && loc->parentLoc && HasAnyKeyword(loc->parentLoc, safeList)) {
			a_why = std::format("parent {} safe", Util::NameOf(loc->parentLoc));
			return true;
		}
		a_why = "no safe keyword";
		return false;
	}

	RE::TESObjectARMO* Helmet::WornHeadgear(RE::PlayerCharacter* a_player) const
	{
		using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;
		for (const auto slot : { Slot::kHead, Slot::kHair, Slot::kCirclet, Slot::kModMouth, Slot::kModFaceJewelry }) {
			auto* armor = a_player->GetWornArmor(slot);
			if (!armor || (ignoreKeyword && armor->HasKeyword(ignoreKeyword)) ||
				(circletKeyword && armor->HasKeyword(circletKeyword))) {
				continue;
			}
			for (const auto* keyword : headgearKeywords) {
				if (armor->HasKeyword(keyword)) {
					return armor;
				}
			}
		}
		return nullptr;
	}

	void Helmet::Tick()
	{
		Util::WarnIfSIModuleOn("HelmetToggle.enabled", "/MCP/modules/HelmetToggle/enabled");
		auto* player = Util::Player();
		if (!ready || !player) {
			LogGate("helmet toggle not ready");
			off.Update(false, {});
			on.Update(false, {});
			return;
		}

		const auto* loc = player->GetCurrentLocation();
		const RE::FormID locID = loc ? loc->GetFormID() : 0;
		if (locID != location) {
			location = locID;
			if (dismissed) {
				dismissed = false;
				Log("location changed: helmet prompt offered again");
			}
		}

		const float type = Value(hotkeyType);
		if (type != 1.0f && !warnedType) {
			warnedType = true;
			Log("WARN Helmet Toggle hotkey type is {} (CIGAR presses the simple type 1 only): no helmet prompt", type);
		}

		const bool hidden = Value(helmetState) > 0.0f;
		auto* worn = hidden ? nullptr : WornHeadgear(player);
		std::string why;
		const bool safe = Safe(player, why);
		const bool combat = player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();

		if (pending && Clock::now() >= checkAt) {
			const bool nowOn = Value(helmetState) == 0.0f;
			if (nowOn == wantOn) {
				pending = false;
				Log("helmet {} (state {})", wantOn ? "on" : "off", Value(helmetState));
			} else if (!pressedTwice) {
				pressedTwice = true;
				checkAt = Clock::now() + kCheckDelay;
				Log("helmet state {} after the press; Helmet Toggle's remembered state disagreed, pressing again",
					Value(helmetState));
				Press();
			} else {
				pending = false;
				dismissed = true;
				Log("WARN helmet state {} after two presses (wanted {}); hidden until the location changes",
					Value(helmetState), wantOn ? "on" : "off");
				Util::Notify("CIGAR: 투구 전환 확인 실패. 로그 확인");
			}
		}

		LogGate(std::format("type={} state={} worn={} loc={} safe={} ({}) combat={} movable={} dismissed={} pending={}",
			type, Value(helmetState), worn ? Util::NameOf(worn) : "-"s, loc ? Util::NameOf(loc) : "-"s, safe, why,
			combat, movable, dismissed, pending));

		const bool can = type == 1.0f && !combat && movable && !dismissed && !pending;
		off.Update(can && safe && worn != nullptr, [] { return "투구 벗기 (길게)"s; });
		on.Update(can && !safe && hidden, [] { return "투구 쓰기 (길게)"s; });
	}

	bool Helmet::Press()
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm || !quest) {
			Log("PressHotkey not requested: vm={} quest={}", vm != nullptr, quest != nullptr);
			return false;
		}
		auto* args = RE::MakeFunctionArguments();
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new PressResult() };
		const bool queued = vm->DispatchMethodCall2(Util::Handle(quest), kMCMScript, "PressHotkey", args, callback);
		Log("HT_MCM.PressHotkey requested queued={}", queued);
		return queued;
	}

	void Helmet::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kHelmetOff && a_eventID != kHelmetOn) {
			return;
		}
		wantOn = a_eventID == kHelmetOn;
		if (!Press()) {
			Util::Notify("CIGAR: 투구 전환 호출 실패. 로그 확인");
			return;
		}
		pending = true;
		pressedTwice = false;
		checkAt = Clock::now() + kCheckDelay;
		Log("accepted {}: state {} before the press", wantOn ? "투구 쓰기" : "투구 벗기", Value(helmetState));
	}

	void Helmet::OnDeclined(std::uint16_t a_eventID)
	{
		if (a_eventID != kHelmetOff && a_eventID != kHelmetOn) {
			return;
		}
		dismissed = true;
		Log("declined: hidden until the location changes");
	}

	void Helmet::OnDisabled()
	{
		dismissed = false;
		pending = false;
	}
}
