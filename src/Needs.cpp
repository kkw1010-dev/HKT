#include "Needs.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Private Needs - Orgasm 1.10.2 (Korean). PNO_Config carries both PNO_ConfigScript (MCM, keys,
		// fill amounts) and PNO_Utilityscript (UrinateAndDefecate, IsInSexScene); PNO_MainQuest keeps
		// the bladder/bowel levels (content / size * 5, capped at 5).
		constexpr auto kPlugin = "Private Needs - Orgasm.esp"sv;
		constexpr RE::FormID kConfigQuestID = 0x87C;   // PNO_Config
		constexpr RE::FormID kMainQuestID = 0x87B;     // PNO_MainQuest
		constexpr RE::FormID kExcreteEffectID = 0x853;  // PN_ExcreteAnimHandleMgef, active while PNO excretes
		constexpr auto kConfigScript = "PNO_ConfigScript";
		constexpr auto kUtilityScript = "PNO_Utilityscript";
		constexpr auto kMainScript = "PNO_QF_MainQuest";
		// PNO_QF_MainQuest's bladderSize / bowelSize (autoreadonly, so not readable as variables).
		constexpr float kBladderSize = 600.0f;
		constexpr float kBowelSize = 1440.0f;
		// PNO_Utilityscript.UrinateAndDefecate's excrete type: 1 urinate; 3 is what PNO's excrete key
		// and its needs menu send (defecate, and urinate too when the bladder has anything).
		constexpr std::int32_t kTypeUrinate = 1;
		constexpr std::int32_t kTypeExcrete = 3;

		constexpr auto kPromptOnlyTarget = "privateneeds"sv;
		struct KeyVar
		{
			const char* name;
			const char* label;
			bool property;  // an Auto property (else a script variable)
		};
		// PNO_ConfigScript.mapKey registers these six; OnKeyDown acts on them.
		constexpr std::array kKeyVars{
			KeyVar{ "Universal_keyCode", "menu", true },
			KeyVar{ "CheckNeeds_keyCode", "check", true },
			KeyVar{ "Urinate_KeyCode", "urinate", false },
			KeyVar{ "Excrete_KeyCode", "excrete", false },
			KeyVar{ "Wetself_KeyCode", "wetself", false },
			KeyVar{ "Toilet_Keycode", "toilet", false },
		};

		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;

		// UrinateAndDefecate waits 0.2 s and casts its handler, which strips and plays before the
		// fill resets; give it time before the gate is read again.
		constexpr auto kQuietAfterStart = 3s;
		constexpr auto kStartWindow = 3s;
		constexpr auto kJournalDelay = 1500ms;

		class StartResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			explicit StartResult(std::int32_t a_type) :
				type(a_type) {}

			void operator()(RE::BSScript::Variable) override
			{
				Needs::GetSingleton()->Log("UrinateAndDefecate({}) returned", type);
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		private:
			std::int32_t type;
		};

		class NoResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable) override {}
			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};

		std::int64_t NowTicks()
		{
			return std::chrono::steady_clock::now().time_since_epoch().count();
		}

		std::string KeyText(std::int32_t a_code)
		{
			return a_code >= 0 ? std::to_string(a_code) : "-"s;
		}
	}

	Needs::Needs()
	{
		// The prompt stays for as long as the level holds; an accept that PNO refused (privacy,
		// worn items) must not leave it gone.
		urinate.SetRepeat(true);
		defecate.SetRepeat(true);
	}

	Needs* Needs::GetSingleton()
	{
		static Needs singleton;
		return &singleton;
	}

	void Needs::RegisterEvents()
	{
		if (auto* ui = RE::UI::GetSingleton()) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(this);
		}
	}

	RE::BSEventNotifyControl Needs::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (a_event && !a_event->opening && a_event->menuName == RE::JournalMenu::MENU_NAME) {
			// PNO's OnConfigClose and a profile load run as Papyrus after the menu closes.
			keyCheckAt = NowTicks() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(kJournalDelay).count();
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	void Needs::OnGameLoaded()
	{
		urinate.Reset();
		defecate.Reset();
		lastGate.clear();
		active = false;
		config = nullptr;
		utility = nullptr;
		main = nullptr;
		offeredBladder = -1;
		offeredBowel = -1;
		quietUntil = {};
		checking = false;
		keyCheckAt = 0;
		warnedStopped = false;

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kPlugin)) {
			Log("Private Needs - Orgasm not found; the needs prompts are off");
			return;
		}
		configQuest = handler->LookupForm<RE::TESQuest>(kConfigQuestID, kPlugin);
		mainQuest = handler->LookupForm<RE::TESQuest>(kMainQuestID, kPlugin);
		excreteEffect = handler->LookupForm<RE::EffectSetting>(kExcreteEffectID, kPlugin);
		config = Util::ScriptObject(configQuest, kConfigScript);
		utility = Util::ScriptObject(configQuest, kUtilityScript);
		main = Util::ScriptObject(mainQuest, kMainScript);
		sexlabAnimating = handler->LookupModByName(kSexLabPlugin) ? handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin) : nullptr;

		Log("PNO config={} utility={} main={} excreteEffect={} sexlab={}", static_cast<bool>(config), static_cast<bool>(utility),
			static_cast<bool>(main), excreteEffect != nullptr, sexlabAnimating != nullptr);
		if (!config || !utility || !main || !excreteEffect) {
			Log("WARN PNO found but its quests/scripts did not resolve; the needs prompts are off");
			Util::Notify(Text::L("CIGAR: Private Needs 연동 실패. 용변 프롬프트 비활성", "CIGAR: Private Needs link failed. Relief prompts off"));
			return;
		}
		active = true;
		ApplyKeyMode();
		Log("ready");
	}

	void Needs::ApplyKeyMode()
	{
		if (!config) {
			Log("key check: PNO not loaded");
			return;
		}
		const bool promptOnly = Settings::PromptOnly(kPromptOnlyTarget);
		std::string summary;
		std::string changes;
		for (const auto& key : kKeyVars) {
			auto* var = key.property ? config->GetProperty(key.name) : config->GetVariable(key.name);
			if (!var || !var->IsInt()) {
				Log("WARN PNO key variable {} not found", key.name);
				continue;
			}
			const auto current = var->GetSInt();
			auto wanted = current;
			if (promptOnly) {
				if (current >= 0) {
					// Remember the player's key so switching prompt-only off gives it back.
					if (current < 264) {
						Settings::SetManualKey(kPromptOnlyTarget, key.label, current);
					}
					wanted = -1;
				}
			} else if (current < 0) {
				wanted = Settings::ManualKey(kPromptOnlyTarget, key.label);
				// Given back once; a key unbound later in PNO's MCM stays unbound.
				Settings::SetManualKey(kPromptOnlyTarget, key.label, -1);
			}
			if (wanted != current) {
				var->SetSInt(wanted);
				changes += std::format(" {} {}->{}", key.label, KeyText(current), KeyText(wanted));
			}
			if (wanted >= 0) {
				summary += std::format("{}{}={}", summary.empty() ? "" : " ", key.label, wanted);
			}
		}
		{
			std::scoped_lock lock(keyLock);
			keySummary = summary;
		}
		if (changes.empty()) {
			Log("PNO keys unchanged (prompt-only={}): {}", promptOnly, summary.empty() ? "none bound"s : summary);
			return;
		}
		// mapKey unregisters every key and registers the six variables again.
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* args = RE::MakeFunctionArguments();
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new NoResult() };
		const bool queued = vm->DispatchMethodCall2(Util::Handle(configQuest), kConfigScript, "mapKey", args, callback);
		Log("{}PNO keys changed (prompt-only={}):{}; mapKey queued={}", queued ? "" : "WARN ", promptOnly, changes, queued);
	}

	std::string Needs::KeySummary() const
	{
		std::scoped_lock lock(keyLock);
		return keySummary;
	}

	Needs::State Needs::Read(RE::PlayerCharacter* a_player, std::string& a_gate) const
	{
		State s;
		s.running = mainQuest->IsRunning();
		s.bladderOn = Util::ScriptBool(config, "bladdertoggleVal");
		s.bowelOn = Util::ScriptBool(config, "boweltoggleVal");
		s.bladderLevel = Util::ScriptInt(main, "bladder_lastlevel");
		s.bowelLevel = Util::ScriptInt(main, "bowel_lastlevel");
		s.bladderPercent = 100.0f * Util::ScriptFloat(config, "bladdercontent") / kBladderSize;
		s.bowelPercent = 100.0f * Util::ScriptFloat(config, "bowelcontent") / kBowelSize;
		auto* target = a_player->AsMagicTarget();
		s.excreting = target && target->HasMagicEffect(excreteEffect);

		const bool combat = a_player->IsInCombat();
		const auto* state = a_player->AsActorState();
		const bool swimming = state && state->IsSwimming();
		const bool seated = state && state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal;
		const bool mounted = a_player->IsOnMount();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		// PNO turns its excrete keys into an orgasm during a scene; the prompts stay out of scenes.
		const bool scene = Util::ScriptBool(utility, "IsInSexScene") || (sexlabAnimating && a_player->IsInFaction(sexlabAnimating));
		const bool quiet = Clock::now() < quietUntil;
		const int minPercent = Settings::NeedsMinPercent();

		// PNO's own refusals (combat, swimming) plus what would break its animation.
		const bool free = s.running && !s.excreting && !combat && !swimming && !seated && !mounted && movable && !scene && !quiet;
		s.canUrinate = free && s.bladderOn && s.bladderLevel >= 1 && s.bladderPercent >= static_cast<float>(minPercent);
		s.canDefecate = free && s.bowelOn && s.bowelLevel >= 1 && s.bowelPercent >= static_cast<float>(minPercent);

		// The fill changes every PNO update; the gate logs levels only.
		a_gate = std::format("running={} bladder={}:{} bowel={}:{} min={}% excreting={} combat={} swim={} seated={} mounted={} movable={} scene={} quiet={}",
			s.running, s.bladderOn ? "on" : "off", s.bladderLevel, s.bowelOn ? "on" : "off", s.bowelLevel, minPercent, s.excreting, combat,
			swimming, seated, mounted, movable, scene, quiet);
		return s;
	}

	void Needs::CheckAfterStart(const State& a_state)
	{
		if (!checking) {
			return;
		}
		if (a_state.excreting) {
			if (!sawEffect) {
				sawEffect = true;
				Log("PNO excrete handler running");
			}
			return;
		}
		const float fill = checkedPrompt == kUrinate ? a_state.bladderPercent : a_state.bowelPercent;
		if (sawEffect) {
			checking = false;
			// PNO resets the fill in its pee/poop effect; a handler that ended without it refused
			// (privacy, a blocking worn item) and said so in its own notification.
			Log("{}PNO excrete handler ended: {} {:.0f}% -> {:.0f}%", fill < fillBefore ? "" : "WARN ",
				checkedPrompt == kUrinate ? "bladder" : "bowel", fillBefore, fill);
			return;
		}
		if (Clock::now() >= checkDeadline) {
			checking = false;
			Log("WARN PNO did not start excreting within {} s of the request (fill {:.0f}%)",
				std::chrono::duration_cast<std::chrono::seconds>(kStartWindow).count(), fill);
			Util::Notify(Text::L("CIGAR: Private Needs가 용변을 시작하지 않음. 로그 확인", "CIGAR: Private Needs did not start. See the log"));
		}
	}

	void Needs::Tick()
	{
		if (!active) {
			return;
		}
		if (const auto at = keyCheckAt.load(); at != 0 && NowTicks() >= at) {
			keyCheckAt = 0;
			ApplyKeyMode();
		}
		auto* player = Util::Player();
		std::string gate;
		const auto s = Read(player, gate);
		LogGate(std::move(gate));
		CheckAfterStart(s);

		if (!s.running && !warnedStopped) {
			warnedStopped = true;
			Log("WARN PNO's main quest is not running; switch PNO on in its MCM");
			Util::Notify(Text::L("CIGAR: Private Needs 꺼짐. MCM에서 켜야 용변 프롬프트 표시", "CIGAR: Private Needs is off. Enable it in its MCM for relief prompts"));
		}

		// The prompt shows the fill; offer it again when the level changes.
		if (urinate.Offered() && s.bladderLevel != offeredBladder) {
			urinate.Withdraw();
			urinate.Reset();
		}
		if (defecate.Offered() && s.bowelLevel != offeredBowel) {
			defecate.Withdraw();
			defecate.Reset();
		}
		offeredBladder = s.bladderLevel;
		offeredBowel = s.bowelLevel;
		urinate.Update(s.canUrinate, [&s] { return Text::F("소변 보기 ({:.0f}%)", "Urinate ({:.0f}%)", s.bladderPercent); });
		defecate.Update(s.canDefecate, [&s] { return Text::F("대변 보기 ({:.0f}%)", "Defecate ({:.0f}%)", s.bowelPercent); });
	}

	void Needs::OnAccepted(std::uint16_t a_eventID)
	{
		if (!active || (a_eventID != kUrinate && a_eventID != kDefecate)) {
			return;
		}
		auto* player = Util::Player();
		std::string gate;
		const auto s = Read(player, gate);
		const bool pee = a_eventID == kUrinate;
		if (pee ? !s.canUrinate : !s.canDefecate) {
			Log("accept ignored, no longer live: {}", gate);
			return;
		}
		urinate.Withdraw();
		defecate.Withdraw();
		const auto type = pee ? kTypeUrinate : kTypeExcrete;
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* args = RE::MakeFunctionArguments(std::int32_t{ type }, static_cast<RE::Actor*>(nullptr), static_cast<RE::TESObjectREFR*>(nullptr));
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new StartResult(type) };
		const bool queued = vm->DispatchMethodCall2(Util::Handle(configQuest), kUtilityScript, "UrinateAndDefecate", args, callback);
		quietUntil = Clock::now() + kQuietAfterStart;
		checking = queued;
		sawEffect = false;
		checkDeadline = Clock::now() + kStartWindow;
		checkedPrompt = a_eventID;
		fillBefore = pee ? s.bladderPercent : s.bowelPercent;
		Log("{}UrinateAndDefecate({}) requested: bladder {}:{:.0f}% bowel {}:{:.0f}% queued={}", queued ? "" : "WARN ", type,
			s.bladderLevel, s.bladderPercent, s.bowelLevel, s.bowelPercent, queued);
		if (!queued) {
			Util::Notify(Text::L("CIGAR: Private Needs 호출 실패. 로그 확인", "CIGAR: Private Needs call failed. See the log"));
		}
	}
}
