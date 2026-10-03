#include "VoiceAnswer.h"

#include <numbers>

#include "Text.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Starting value (mine): about 14 m. Enemies further off are not part of this fight's crowd yet.
		constexpr float kFightRange = 1000.0f;
		// A friend this far either side of the heading is "in front" (docs/047, approved as D57).
		constexpr float kFrontCone = 45.0f;
		// Kyne's Peace calms animals up to this level (UESP).
		constexpr int kKynesPeaceLevel = 20;
		// Dismay's level reach by the number of words unlocked (UESP): none, 1, 2, 3.
		constexpr std::array<int, 4> kDismayLevel{ 0, 7, 15, 24 };
		// Skyrim.esm shouts: Unrelenting Force, Slow Time, Kyne's Peace, Dismay; DwarvenCenturionRace.
		constexpr RE::FormID kUnrelentingForce = 0x013E07;
		constexpr RE::FormID kSlowTime = 0x048AC9;
		constexpr RE::FormID kKynesPeace = 0x07097E;
		constexpr RE::FormID kDismay = 0x02395A;
		constexpr RE::FormID kCenturionRace = 0x0131F1;

		std::string Crowd(const VoiceAnswer::Fight& a_fight)
		{
			return a_fight.enemies >= 2 ? ""s : std::format("{} enemy in combat", a_fight.enemies);
		}

		std::string FriendAhead(const VoiceAnswer::Fight& a_fight)
		{
			if (auto why = Crowd(a_fight); !why.empty()) {
				return why;
			}
			return a_fight.friendsAhead > 0 ? ""s : "no friend or bystander in front"s;
		}

		std::string AnimalPack(const VoiceAnswer::Fight& a_fight)
		{
			if (auto why = Crowd(a_fight); !why.empty()) {
				return why;
			}
			if (a_fight.animals < a_fight.enemies) {
				return std::format("{} of {} are animals", a_fight.animals, a_fight.enemies);
			}
			return a_fight.maxLevel <= kKynesPeaceLevel ? ""s : std::format("an animal at level {} (> {})", a_fight.maxLevel, kKynesPeaceLevel);
		}

		std::string Unshakeable(const VoiceAnswer::Fight& a_fight)
		{
			if (auto why = Crowd(a_fight); !why.empty()) {
				return why;
			}
			return a_fight.unshakeable > 0 ? ""s : "no dragon or centurion"s;
		}

		std::string LowLevelCrowd(const VoiceAnswer::Fight& a_fight)
		{
			if (auto why = Crowd(a_fight); !why.empty()) {
				return why;
			}
			if (a_fight.dismayCap == 0) {
				return "no Dismay word unlocked"s;
			}
			if (a_fight.dismayProof > 0) {
				return std::format("{} undead, daedra, automaton or dragon", a_fight.dismayProof);
			}
			return a_fight.maxLevel <= a_fight.dismayCap ? ""s : std::format("an enemy at level {} (> {})", a_fight.maxLevel, a_fight.dismayCap);
		}

		float Degrees(float a_side, float a_ahead)
		{
			return std::atan2(a_side, a_ahead) * 180.0f / std::numbers::pi_v<float>;
		}
	}

	VoiceAnswer::VoiceAnswer()
	{
		// A combat prompt: a single press (check_prompt_rules.py lists it).
		prompt.SetPromptType(SkyPromptAPI::kSinglePress);
		// From the top; the first rule that holds and whose shout can be used wins (D57).
		answers.push_back({ "friend-ahead", kSlowTime, &FriendAhead });
		answers.push_back({ "animal-pack", kKynesPeace, &AnimalPack });
		answers.push_back({ "unshakeable", kSlowTime, &Unshakeable });
		answers.push_back({ "low-level-crowd", kDismay, &LowLevelCrowd });
		answers.push_back({ "crowd", kUnrelentingForce, &Crowd });
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
			list += std::format("{}{} -> {} ({:08X})", list.empty() ? "" : ", ", answer.rule, answer.shout ? Util::NameOf(answer.shout) : "MISSING"s,
				answer.shoutID);
		}
		dismay = RE::TESForm::LookupByID<RE::TESShout>(kDismay);
		readyAt.clear();
		individualCooldowns = GetModuleHandleW(L"TISC.dll") != nullptr;
		Log("shout cooldowns: {}", individualCooldowns ? "per shout (TISC loaded): each shout's own timer is remembered" : "shared (vanilla)");
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
			const bool dragon = actor->HasKeywordString("ActorTypeDragon");
			const auto* race = actor->GetRace();
			const bool centurion = race && race->GetFormID() == kCenturionRace;
			fight.animals += actor->HasKeywordString("ActorTypeAnimal") ? 1 : 0;
			fight.unshakeable += dragon || centurion ? 1 : 0;
			fight.dismayProof += dragon || actor->HasKeywordString("ActorTypeUndead") || actor->HasKeywordString("ActorTypeDaedra") ||
			                             actor->HasKeywordString("ActorTypeDwarven") ?
			                         1 :
			                         0;
			fight.maxLevel = std::max<int>(fight.maxLevel, actor->GetLevel());
		}
		// Friends and bystanders in front: the same walk as Util::NearbyHostiles (bleeding read through
		// AsActorState, see r19), keeping the non-hostile ones.
		if (auto* lists = RE::ProcessLists::GetSingleton()) {
			const auto origin = a_player->GetPosition();
			const float heading = a_player->GetAngleZ();
			for (auto& handle : lists->highActorHandles) {
				const auto ptr = handle.get();
				auto* other = ptr.get();
				if (!other || other == a_player || other->IsDead() || !other->Is3DLoaded() || other->IsHostileToActor(a_player)) {
					continue;
				}
				const auto to = other->GetPosition() - origin;
				if (origin.GetDistance(other->GetPosition()) > kFightRange) {
					continue;
				}
				const float ahead = to.x * std::sin(heading) + to.y * std::cos(heading);
				const float side = to.x * std::cos(heading) - to.y * std::sin(heading);
				if (std::abs(Degrees(side, ahead)) <= kFrontCone) {
					++fight.friendsAhead;
					if (fight.friendAhead.empty()) {
						fight.friendAhead = Util::NameOf(other);
					}
				}
			}
		}
		// Dismay's reach: the words of it the player has unlocked.
		if (dismay && a_player->HasShout(dismay)) {
			int words = 0;
			for (const auto& variation : dismay->variations) {
				if (variation.word && (variation.word->GetFormFlags() & RE::TESForm::RecordFlags::kUnlocked) != 0) {
					++words;
				}
			}
			fight.dismayCap = kDismayLevel[std::min(words, 3)];
		}
		return fight;
	}

	bool VoiceAnswer::Ready(RE::TESShout* a_shout, const RE::TESShout* a_equipped, float a_recovery, std::string& a_why) const
	{
		if (!individualCooldowns) {
			if (a_recovery > 0.0f) {
				a_why = std::format("voice recovering ({:.0f} s)", a_recovery);
				return false;
			}
			return true;
		}
		if (a_shout == a_equipped) {
			return a_recovery <= 0.0f;
		}
		const auto it = readyAt.find(a_shout);
		if (it != readyAt.end()) {
			const auto left = std::chrono::duration<float>(it->second - std::chrono::steady_clock::now()).count();
			if (left > 0.0f) {
				a_why = std::format("{} recovering ({:.0f} s, TISC)", Util::NameOf(a_shout), left);
				return false;
			}
		}
		return true;
	}

	const VoiceAnswer::Answer* VoiceAnswer::Pick(RE::PlayerCharacter* a_player, const Fight& a_fight, std::string& a_rules) const
	{
		const auto* equipped = a_player->GetActorRuntimeData().selectedPower;
		const Answer* pick = nullptr;
		bool done = false;
		for (const auto& answer : answers) {
			std::string verdict;
			if (pick || done) {
				verdict = "-";
			} else if (auto why = answer.check(a_fight); !why.empty()) {
				verdict = why;
			} else if (!answer.shout || !a_player->HasShout(answer.shout)) {
				verdict = std::format("holds, but {} is not known", answer.shout ? Util::NameOf(answer.shout) : "the shout"s);
			} else if (std::string busy; equipped != answer.shout && !Ready(answer.shout, equipped, lastRecovery, busy)) {
				verdict = std::format("holds, but {}", busy);
			} else if (equipped == answer.shout) {
				// The best answer is already in the slot: nothing lower down is offered (r24: after Kyne's Peace
				// was equipped the next tick offered Dismay, after Slow Time it offered Unrelenting Force).
				verdict = std::format("holds, {} already equipped: nothing else offered", Util::NameOf(answer.shout));
				done = true;
			} else {
				verdict = std::format("PICKED {}", Util::NameOf(answer.shout));
				pick = &answer;
			}
			a_rules += std::format("{}{}: {}", a_rules.empty() ? "" : " | ", answer.rule, verdict);
		}
		return pick;
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
		// Vanilla: one voice recovery for every shout; while it runs, equipping one gives nothing to do yet.
		// TISC: it is the equipped shout's own timer; it is remembered for that shout and each answer is
		// checked against its own (Ready).
		const float recovery = player->GetVoiceRecoveryTime();
		lastRecovery = recovery;
		if (individualCooldowns) {
			const auto* power = player->GetActorRuntimeData().selectedPower;
			if (const auto* equippedShout = power ? power->As<RE::TESShout>() : nullptr) {
				if (recovery > 0.0f) {
					readyAt[equippedShout] = std::chrono::steady_clock::now() +
					                         std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(recovery));
				} else {
					readyAt.erase(equippedShout);
				}
			}
		}

		Fight fight;
		std::string why;
		std::string rules;
		const Answer* pick = nullptr;
		if (!combat) {
			why = "not in combat";
		} else if (!movable || busy) {
			why = "busy";
		} else {
			fight = Look(player);
			if (!individualCooldowns && recovery > 0.0f) {
				why = std::format("voice recovering ({:.0f} s)", recovery);
			} else {
				pick = Pick(player, fight, rules);
				if (!pick) {
					why = "no rule picked a usable shout";
				}
			}
		}
		const bool held = pick && pick == candidate;
		candidate = pick;

		LogGate(std::format("combat={} movable={} busy={} recovery={:.0f} listed={} hostile={} bleeding={} enemies={} sight(player->them={}, "
		                    "them->player={}) animals={} unshakeable={} dismayProof={} maxLevel={} friendsAhead={}{} dismayCap={} pick={} ({}){}",
			combat, movable, busy, recovery, fight.listed, fight.hostile, fight.bleeding, fight.enemies, fight.playerSees, fight.seesPlayer,
			fight.animals, fight.unshakeable, fight.dismayProof, fight.maxLevel, fight.friendsAhead,
			fight.friendAhead.empty() ? "" : std::format(" ({})", fight.friendAhead), fight.dismayCap,
			pick && pick->shout ? Util::NameOf(pick->shout) : "-"s, pick ? pick->rule : why, rules.empty() ? "" : " rules: " + rules));

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
		Log("equipped {} (rule {}): voice slot was {}, now {}", Util::NameOf(answer->shout), answer->rule, before ? Util::NameOf(before) : "-"s,
			after ? Util::NameOf(after) : "-"s);
		if (after != answer->shout) {
			Log("WARN the voice slot does not hold the shout right after equipping");
			Util::Notify(Text::L("CIGAR: 용언 장착 확인 실패. 로그 확인", "CIGAR: The shout was not equipped. See the log"));
		}
	}
}
