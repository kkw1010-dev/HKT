#include "BaboKey.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kBaboPlugin = "BaboInteractiveDia.esp"sv;
		constexpr auto kMonitorScript = "BaboDiaMonitorScript";
		constexpr auto kConfigScript = "BaboDialogueConfigMenu";
		constexpr auto kKidnapScript = "BaboKidnapEvenScript";
		constexpr RE::FormID kMonitorQuestID = 0x7E22B8;  // BaboMonitorScript

		// BaboDiaMonitorScript.OnKeyDown hands the key to BaboKidnapEvenScript.KeyPress() once the
		// kidnap quest reaches stage 8, where BaboDialogue shows its own hotkey tutorial. Stage 250
		// releases the player and 255 shuts the quest down.
		constexpr std::uint16_t kFirstKeyStage = 8;
		constexpr std::uint16_t kReleaseStage = 250;

		// KeyPress() acts in BaboKidnapCabin, BaboKidnapBanditCave (while bCaptured),
		// BaboSlaverCabin and baboslaverinterval, and returns false at once in every other state.
		// The prompt stays up in the kidnap room regardless: the state changes during rests and
		// scenes, and a press in a quiet state is harmless. The state is still logged.

		std::string CellName(const RE::TESObjectCELL* a_cell)
		{
			if (!a_cell) {
				return "-";
			}
			const char* id = a_cell->GetFormEditorID();
			if (id && *id) {
				return id;
			}
			return std::format("{:08X}", a_cell->GetFormID());
		}

		class KeyResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable) override
			{
				BaboKey::GetSingleton()->Log("OnKeyDown returned");
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};
	}

	BaboKey* BaboKey::GetSingleton()
	{
		static BaboKey singleton;
		return &singleton;
	}

	bool BaboKey::Resolve()
	{
		monitor = nullptr;
		configQuest = nullptr;
		kidnap = nullptr;
		npcAnimating = nullptr;
		tiedUp = nullptr;
		scenario = nullptr;
		centerMarker = nullptr;

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kBaboPlugin)) {
			return false;
		}
		monitor = handler->LookupForm<RE::TESQuest>(kMonitorQuestID, kBaboPlugin);

		const auto monitorScript = Util::ScriptObject(monitor, kMonitorScript);
		configQuest = Util::ScriptProperty<RE::TESQuest>(monitorScript, "BDConfig");
		kidnap = Util::ScriptProperty<RE::TESQuest>(monitorScript, "BaboKidnapEvent");
		npcAnimating = Util::ScriptProperty<RE::TESFaction>(monitorScript, "BaboNPCAnimating");

		const auto kidnapScript = Util::ScriptObject(kidnap, kKidnapScript);
		tiedUp = Util::ScriptProperty<RE::TESGlobal>(kidnapScript, "BaboKidnapTiedUp");
		scenario = Util::ScriptProperty<RE::TESGlobal>(kidnapScript, "BaboKidnapScenarioe");
		centerMarker = Util::ScriptProperty<RE::BGSRefAlias>(kidnapScript, "CenterMarkerPlayer");
		const bool hasCaptured = kidnapScript && kidnapScript->GetVariable("bCaptured");

		Log("Babo monitor={} script={} config={} kidnap={} kidnapScript={} npcAnimating={} tiedUp={} scenario={} centerMarker={} bCaptured={}{} key={}",
			monitor ? std::format("{:08X}", monitor->GetFormID()) : "-", static_cast<bool>(monitorScript),
			configQuest != nullptr, kidnap ? std::format("{:08X}", kidnap->GetFormID()) : "-",
			static_cast<bool>(kidnapScript), npcAnimating != nullptr, tiedUp != nullptr, scenario != nullptr,
			centerMarker != nullptr, hasCaptured, Util::DescribeScenes(), NotificationKey());
		return monitorScript && kidnapScript && npcAnimating && centerMarker && hasCaptured;
	}

	std::int32_t BaboKey::NotificationKey() const
	{
		// BDConfig.NotificationKey is an auto property; VirtualMachine::GetPropertyValue does not
		// read it (BaboPrismUI finding), Object::GetProperty does.
		const auto config = Util::ScriptObject(configQuest, kConfigScript);
		const auto* var = config ? config->GetProperty("NotificationKey") : nullptr;
		return var && var->IsInt() ? var->GetSInt() : -1;
	}

	void BaboKey::OnGameLoaded()
	{
		act.Reset();
		lastGate.clear();

		if (!Resolve()) {
			const bool present = monitor != nullptr;
			kidnap = nullptr;
			if (present) {
				// BaboDialogue is installed but its scripts no longer match what this module reads.
				Log("WARN BaboDialogue found but its kidnap scripts did not resolve; the hotkey prompt is off");
				if (!warnedBroken) {
					warnedBroken = true;
					Util::Notify(Text::L("CIGAR: 바보 납치 연동 실패. 핫키 프롬프트 비활성", "CIGAR: BaboDialogue kidnap link failed. Hotkey prompt off"));
				}
			} else {
				Log("BaboDialogue not found; the hotkey prompt is off");
			}
			return;
		}
		Log("ready");
	}

	bool BaboKey::KeyIsLive(std::string& a_gate)
	{
		if (!kidnap->IsRunning()) {
			a_gate = "kidnap=off";
			return false;
		}

		auto* player = Util::Player();
		const auto stage = kidnap->GetCurrentStageID();
		// The quest idles at stage 0 between kidnaps; only the stage matters then.
		if (stage < kFirstKeyStage || stage >= kReleaseStage) {
			a_gate = std::format("kidnap=idle stage={}", stage);
			return false;
		}
		const auto script = Util::ScriptObject(kidnap, kKidnapScript);
		const std::string_view state = script ? script->currentState.c_str() : "";
		const auto* captured = script ? script->GetVariable("bCaptured") : nullptr;
		const bool isCaptured = captured && captured->IsBool() && captured->GetBool();

		const auto* playerCell = player->GetParentCell();
		const auto* center = centerMarker->GetReference();
		const auto* roomCell = center ? center->GetParentCell() : nullptr;
		const bool inRoom = playerCell && playerCell == roomCell;

		const bool babo = player->IsInFaction(npcAnimating);
		const bool sexlab = Util::InScene(player);

		a_gate = std::format("kidnap=on stage={} state='{}' captured={} tied={} scenario={} cell={} room={} babo-anim={} scene={}",
			stage, state, isCaptured, tiedUp ? tiedUp->value : -1.0f, scenario ? scenario->value : -1.0f,
			CellName(playerCell), CellName(roomCell), babo, sexlab);

		// Player controls are not checked: BaboDialogue's StuckControl() keeps a tied player's
		// controls off, and the key must still work then.
		return inRoom && !babo && !sexlab;
	}

	void BaboKey::Tick()
	{
		if (!kidnap) {
			return;
		}
		std::string gate;
		const bool live = KeyIsLive(gate);
		LogGate(std::move(gate));
		act.Update(live, [] { return std::string(Text::L("행동 선택", "Choose Action")); });
	}

	void BaboKey::OnAccepted(std::uint16_t a_eventID)
	{
		if (!kidnap || a_eventID != kAct) {
			return;
		}
		// Re-check: the scene may have moved on while the prompt was up.
		std::string gate;
		if (!KeyIsLive(gate)) {
			Log("accept ignored, key no longer live: {}", gate);
			return;
		}
		const auto key = NotificationKey();
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = vm->GetObjectHandlePolicy();
		const auto handle = policy->GetHandleForObject(monitor->GetFormType(), monitor);
		// OnKeyDown compares the argument with BDConfig.NotificationKey, so pass that value.
		auto* args = RE::MakeFunctionArguments(static_cast<std::int32_t>(key));
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new KeyResult() };
		const bool queued = vm->DispatchMethodCall2(handle, kMonitorScript, "OnKeyDown", args, callback);
		Log("OnKeyDown({}) requested queued={} ({})", key, queued, gate);
	}
}
