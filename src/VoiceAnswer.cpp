#include "VoiceAnswer.h"

#include "Text.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Starting value (mine): about 14 m. Enemies further off are not part of this fight's crowd yet.
		constexpr float kFightRange = 1000.0f;

		bool TwoOrMore(const VoiceAnswer::Fight& a_fight) { return a_fight.enemies >= 2; }
	}

	VoiceAnswer::VoiceAnswer()
	{
		// A combat prompt: a single press (check_prompt_rules.py lists it).
		prompt.SetPromptType(SkyPromptAPI::kSinglePress);
		// Skyrim.esm MAG_UnrelentingForceShout. Further answers go here (docs/046 lists candidates).
		answers.push_back({ 0x013E07, "two or more enemies within range", &TwoOrMore });
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
		// r19: an own loop over the high-process list counted no one (near=0 for a whole fight) while
		// WeaponSwap, through Util::NearbyHostiles, saw its target at 73 units in the same tick. The crowd is
		// now counted through that same proven helper; the list's size and the actors it skipped are logged.
		if (auto* lists = RE::ProcessLists::GetSingleton()) {
			fight.listed = static_cast<int>(lists->highActorHandles.size());
		}
		for (auto* actor : Util::NearbyHostiles(a_player, kFightRange)) {
			++fight.hostile;
			if (actor->AsActorState()->IsBleedingOut()) {
				++fight.bleeding;
				continue;
			}
			if (!actor->IsInCombat()) {
				continue;
			}
			++fight.enemies;
			bool unused = false;
			fight.playerSees += a_player->HasLineOfSight(actor, unused) ? 1 : 0;
			fight.seesPlayer += actor->HasLineOfSight(a_player, unused) ? 1 : 0;
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
		} else {
			fight = Look(player);
			if (recovery > 0.0f) {
				why = std::format("voice recovering ({:.0f} s)", recovery);
			} else {
				pick = Pick(player, fight, why);
			}
		}
		const bool held = pick && pick == candidate;
		candidate = pick;

		LogGate(std::format("combat={} movable={} busy={} recovery={:.0f} listed={} hostile={} bleeding={} enemies={} sight(player->them={}, "
		                    "them->player={}) pick={} ({})",
			combat, movable, busy, recovery, fight.listed, fight.hostile, fight.bleeding, fight.enemies, fight.playerSees, fight.seesPlayer,
			pick && pick->shout ? Util::NameOf(pick->shout) : "-"s, pick ? "ready" : why));

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
