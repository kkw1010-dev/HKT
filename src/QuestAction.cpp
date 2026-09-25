#include "QuestAction.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Skyrim.esm: MQ105 The Way of the Voice, MAG_UnrelentingForceShout, MAG_WhirlwindSprintShout.
		constexpr RE::FormID kMQ105ID = 0x0242BA;
		constexpr RE::FormID kUnrelentingForceID = 0x013E07;
		constexpr RE::FormID kWhirlwindSprintID = 0x02F7BA;
		// MQ105 objectives: 20 "Demonstrate your Unrelenting Force Shout" (to Arngeir), 40 the same at
		// the targets, 60 "Demonstrate your Whirlwind Sprint Shout".
		constexpr std::uint16_t kShowForce = 20;
		constexpr std::uint16_t kForceTargets = 40;
		constexpr std::uint16_t kShowSprint = 60;
	}

	QuestAction::QuestAction()
	{
		prompt.SetPromptType(SkyPromptAPI::kHold);
	}

	QuestAction* QuestAction::GetSingleton()
	{
		static QuestAction singleton;
		return &singleton;
	}

	void QuestAction::OnGameLoaded()
	{
		prompt.Reset();
		lastGate.clear();
		wayOfTheVoice = RE::TESForm::LookupByID<RE::TESQuest>(kMQ105ID);
		unrelentingForce = RE::TESForm::LookupByID<RE::TESShout>(kUnrelentingForceID);
		whirlwindSprint = RE::TESForm::LookupByID<RE::TESShout>(kWhirlwindSprintID);
		Log("ready: MQ105={} unrelentingForce={} whirlwindSprint={}", wayOfTheVoice != nullptr,
			unrelentingForce ? Util::NameOf(unrelentingForce) : "-"s, whirlwindSprint ? Util::NameOf(whirlwindSprint) : "-"s);
	}

	RE::TESShout* QuestAction::Wanted(std::uint16_t& a_objective) const
	{
		a_objective = 0;
		if (!wayOfTheVoice || !wayOfTheVoice->IsRunning()) {
			return nullptr;
		}
		for (const auto* objective : wayOfTheVoice->objectives) {
			if (!objective || objective->state.get() != RE::QUEST_OBJECTIVE_STATE::kDisplayed) {
				continue;
			}
			if (objective->index == kShowForce || objective->index == kForceTargets) {
				a_objective = objective->index;
				return unrelentingForce;
			}
			if (objective->index == kShowSprint) {
				a_objective = objective->index;
				return whirlwindSprint;
			}
		}
		return nullptr;
	}

	void QuestAction::Tick()
	{
		auto* player = Util::Player();
		std::uint16_t objective = 0;
		auto* shout = Wanted(objective);
		const bool known = shout && player->HasShout(shout);
		auto* equipped = player->GetActorRuntimeData().selectedPower;
		const bool already = shout && equipped == shout;
		const bool combat = player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();

		LogGate(std::format("mq105={} objective={} shout={} known={} equipped={} combat={} movable={}",
			wayOfTheVoice && wayOfTheVoice->IsRunning() ? std::to_string(wayOfTheVoice->GetCurrentStageID()) : "-"s,
			objective, shout ? Util::NameOf(shout) : "-"s, known, equipped ? Util::NameOf(equipped) : "-"s, combat, movable));

		const bool available = shout && known && !already && !combat && movable;
		prompt.Update(available, [shout] { return std::format("장착하기 (길게): {}", Util::NameOf(shout)); });
	}

	void QuestAction::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kEquipShout) {
			return;
		}
		auto* player = Util::Player();
		std::uint16_t objective = 0;
		auto* shout = Wanted(objective);
		if (!shout || !player->HasShout(shout)) {
			Log("accept ignored: objective={} shout={}", objective, shout ? Util::NameOf(shout) : "-"s);
			return;
		}
		RE::ActorEquipManager::GetSingleton()->EquipShout(player, shout);
		auto* equipped = player->GetActorRuntimeData().selectedPower;
		Log("equipped shout {} for MQ105 objective {}: voice slot now {}", Util::NameOf(shout), objective,
			equipped ? Util::NameOf(equipped) : "-"s);
		if (equipped != shout) {
			Log("WARN the voice slot does not hold the shout right after equipping");
			Util::Notify("CIGAR: 샤우트 장착 확인 실패. 로그 확인");
		}
	}
}
