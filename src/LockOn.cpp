#include "LockOn.h"

#include "TDMLock.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// After a press, give TDM time to act before offering again, so a lock that finds no target
		// does not re-offer at once.
		constexpr auto kQuietAfterPress = 3s;
		constexpr auto kCheckDelay = 1s;
	}

	LockOn* LockOn::GetSingleton()
	{
		static LockOn singleton;
		return &singleton;
	}

	void LockOn::OnGameLoaded()
	{
		lock.Reset();
		lastGate.clear();
		quietUntil = {};
		checkPending = false;

		TDMLock::Resolve();
		if (!TDMLock::Api()) {
			Log("True Directional Movement not found; this module idles");
			return;
		}
		Log("TDM api ok, lock key {} from {} pressable={}", TDMLock::Key(), TDMLock::KeySource(), TDMLock::Pressable());
		if (!TDMLock::Pressable() && !warnedKey) {
			warnedKey = true;
			Log("WARN TDM lock key {} is not a keyboard or mouse key; the lock-on prompt is off", TDMLock::Key());
			Util::Notify("CIGAR: TDM 록온 키가 키보드/마우스 키가 아님. 록온 프롬프트 비활성");
		}
	}

	void LockOn::FastTick()
	{
		if (!TDMLock::Api()) {
			return;
		}
		auto* player = Util::Player();
		const bool combat = player->IsInCombat();
		const bool locked = TDMLock::Locked();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const auto now = Clock::now();
		const bool quiet = now < quietUntil;
		// Grapple is taking the lock again after a grapple; it presses the same key.
		const bool busy = TDMLock::Busy();

		if (checkPending && now >= checkAt) {
			Log("after lock press: locked={}", locked);
			checkPending = false;
		}

		LogGate(std::format("combat={} locked={} movable={} quiet={} relock={} key={}",
			combat, locked, movable, quiet, busy, TDMLock::Pressable()));

		lock.Update(combat && movable && !quiet && !busy && !locked && TDMLock::Pressable(), [] { return "록온"s; });
	}

	void LockOn::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kLock || !TDMLock::Api()) {
			return;
		}
		// Re-check: pressing TDM's key while locked would unlock instead.
		if (TDMLock::Locked()) {
			Log("accept ignored: lock requested while already locked");
			return;
		}
		const bool pressed = TDMLock::Press();
		Log("lock key {} pressed={}", TDMLock::Key(), pressed);
		const auto now = Clock::now();
		quietUntil = now + kQuietAfterPress;
		checkAt = now + kCheckDelay;
		checkPending = true;
	}
}
