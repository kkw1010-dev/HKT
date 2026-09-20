#pragma once

namespace TDM_API
{
	class IVTDM1;
}

namespace CIGAR::TDMLock
{
	// True Directional Movement's target lock, the one resource the LockOn and Grapple modules
	// share: LockOn offers it, Grapple gives it back after a grapple, and both press the same key.
	// Keeping it here instead of in one module lets either module be switched off (or its target
	// mod be absent) without the other losing the lock.
	//
	// Everything below runs on the game thread only.

	// Requests TDM's API and re-reads its lock key from the MCM files. Runs on every game load, so
	// a key changed in the MCM between loads is seen. Also clears Busy().
	void Resolve();

	// Null when True Directional Movement is not installed.
	TDM_API::IVTDM1* Api();
	// False when TDM is absent.
	bool Locked();

	std::int64_t Key();
	// Which file the key came from, for the log.
	std::string_view KeySource();
	// True when Util::PressKey can send Key() (keyboard or mouse).
	bool Pressable();
	bool Press();

	// Set while a module drives the lock itself (Grapple's re-lock after a grapple), so the lock-on
	// prompt stays off the screen and the two never press the key in the same frame.
	void SetBusy(bool a_busy);
	bool Busy();
}
