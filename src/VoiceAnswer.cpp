#include "VoiceAnswer.h"

#include "Text.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Starting value (mine): about 14 m. Enemies further off are not part of this fight's crowd yet.
		constexpr float kFightRange = 1000.0f;

		bool TwoOrMore(const VoiceAnswer::Fight& a_fight) { return a_fight.enemiesInSight >= 2; }
	}

	VoiceAnswer::VoiceAnswer()
	{
		// A combat prompt: a single press (check_prompt_rules.py lists it).
		prompt.SetPromptType(SkyPromptAPI::kSinglePress);
		// Skyrim.esm MAG_UnrelentingForceShout. Further answers go here (docs/046 lists candidates).
		answers.push_back({ 0x013E07, "two or more enemies in sight", &TwoOrMore });
	}

	VoiceAnswer* VoiceAnswer::GetSingleton()
	{
		static VoiceAnswer singleton;
		return &singleton;
	}

	void VoiceAnswer::OnGameLoaded()
	{
		prompt.Reset();
		lastGate.clear();
		offered = nullptr;
		candidate = nullptr;
		wasInCombat = false;
		std::string list;
		for (auto& answer : answers) {
			answer.shout = RE::TESForm::LookupByID<RE::TESShout>(answer.shoutID);
			list += std::format("{}{} ({:08X}) for {}", list.empty() ? "" : ", ", answer.shout ? Util::NameOf(answer.shout) : "MISSING"s,
				answer.shoutID, answer.situation);
		}
		Log("ready: {}", list);
	}

	VoiceAnswer::Fight VoiceAnswer::Look(RE::PlayerCharacter* a_player) const
	{
		Fight fight;
		auto* lists = RE::ProcessLists::GetSingleton();
		if (!lists) {
			return fight;
		}
		const auto pos = a_player->GetPosition();
		for (auto& handle : lists->highActorHandles) {
			const auto actor = handle.get();
			if (!actor || actor.get() == a_player || actor->IsDead() || !actor->Is3DLoaded() || actor->IsBleedingOut() ||
				!actor->IsInCombat() || !actor->IsHostileToActor(a_player) || actor->GetPosition().GetDistance(pos) > kFightRange) {
				continue;
			}
			bool unused = false;
			if (a_player->HasLineOfSight(actor.get(), unused)) {
				++fight.enemiesInSight;
			}
		}
		return fight;
	}

	const VoiceAnswer::Answer* VoiceAnswer::Pick(RE::PlayerCharacter* a_player, const Fight& a_fight, std::string& a_why) const
	{
		const auto* equipped = a_player->GetActorRuntimeData().selectedPower;
		for (const auto& answer : answers) {
			if (!answer.fits(a_fight)) {
				a_why = std::format("not {}", answer.situation);
				continue;
			}
			if (!answer.shout || !a_player->HasShout(answer.shout)) {
				a_why = std::format("{} not known", answer.shout ? Util::NameOf(answer.shout) : "shout"s);
				continue;
			}
			if (equipped == answer.shout) {
				a_why = std::format("{} already equipped", Util::NameOf(answer.shout));
				continue;
			}
			return &answer;
		}
		return nullptr;
	}

	void VoiceAnswer::Tick()
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		const bool combat = player->IsInCombat();
		if (combat && !wasInCombat) {
			++fightNumber;  // a new fight: a decline from the last one no longer holds
		}
		wasInCombat = combat;

		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const auto* state = player->AsActorState();
		const bool busy = player->IsOnMount() || player->IsInKillMove() || (state && state->IsSwimming()) || Util::InScene(player);
		// Voice recovery is shared by every shout: while it runs, equipping one gives nothing to do yet.
		const float recovery = player->GetVoiceRecoveryTime();

		Fight fight;
		std::string why;
		const Answer* pick = nullptr;
		if (!combat) {
			why = "not in combat";
		} else if (!movable || busy) {
			why = "busy";
		} else if (recovery > 0.0f) {
			why = std::format("voice recovering ({:.0f} s)", recovery);
		} else {
			fight = Look(player);
			pick = Pick(player, fight, why);
		}
		const bool held = pick && pick == candidate;
		candidate = pick;

		LogGate(std::format("combat={} movable={} busy={} recovery={:.0f} enemies={} pick={} ({})", combat, movable, busy, recovery,
			fight.enemiesInSight, pick && pick->shout ? Util::NameOf(pick->shout) : "-"s, pick ? "ready" : why));

		// A different shout is a different prompt (its name is in the text).
		if (prompt.Offered() && offered != pick && pick) {
			prompt.Withdraw();
			prompt.Reset();
		}
		prompt.SetSituation(fightNumber);
		if (held) {
			offered = pick;
		}
		auto* shout = held ? pick->shout : nullptr;
		prompt.Update(held, [shout] { return Text::F("장착하기: {}", "Equip: {}", Util::NameOf(shout)); });
	}

	void VoiceAnswer::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kEquip) {
			return;
		}
		auto* player = Util::Player();
		const auto* answer = offered;
		if (!player || !answer || !answer->shout || !player->HasShout(answer->shout)) {
			Log("accept ignored: no usable shout");
			return;
		}
		auto* before = player->GetActorRuntimeData().selectedPower;
		RE::ActorEquipManager::GetSingleton()->EquipShout(player, answer->shout);
		auto* after = player->GetActorRuntimeData().selectedPower;
		Log("equipped {} ({}): voice slot was {}, now {}", Util::NameOf(answer->shout), answer->situation, before ? Util::NameOf(before) : "-"s,
			after ? Util::NameOf(after) : "-"s);
		if (after != answer->shout) {
			Log("WARN the voice slot does not hold the shout right after equipping");
			Util::Notify(Text::L("CIGAR: 용언 장착 확인 실패. 로그 확인", "CIGAR: The shout was not equipped. See the log"));
		}
	}
}
