#pragma once

// Valhalla Combat modder interface.
// Trimmed from D7ry/valhallaCombat src/include/lib/ValhallaCombatAPI.h (BSD-3-Clause, (c) dTry).
// The V2 interface (isActorStunned) is already in commit f5a9056 of 2022-12-21, the source of the
// installed 1.3.3 DLL (Dec 2022); the execution and input code is unchanged from there to master.
// RequestPluginAPI uses GetModuleHandleW because CIGAR builds with UNICODE.

namespace VAL_API
{
	enum class InterfaceVersion : std::uint8_t
	{
		V1,
		V2
	};

	enum STUNSOURCE
	{
		lightAttack,
		powerAttack,
		bash,
		powerBash,
		timedBlock,
		parry,
		counterAttack
	};

	class IVVAL1
	{
	public:
		virtual void processStunDamage(STUNSOURCE a_source, RE::TESObjectWEAP* a_weapon, RE::Actor* a_aggressor, RE::Actor* a_victim, float a_baseDamage) noexcept = 0;
	};

	class IVVAL2 : public IVVAL1
	{
	public:
		virtual bool getIsPCTimedBlocking() noexcept = 0;
		virtual bool getIsPCPerfectBlocking() noexcept = 0;
		virtual void triggerPcTimedBlockSuccess() noexcept = 0;
		// True while the actor's stun meter is broken: the state in which Valhalla lets it be executed.
		virtual bool isActorStunned(RE::Actor* a_actor) noexcept = 0;
		virtual bool isActorExhausted(RE::Actor* a_actor) noexcept = 0;
	};

	using _RequestPluginAPI = void* (*)(const InterfaceVersion a_interfaceVersion);

	// Call at or after kPostLoad.
	[[nodiscard]] inline IVVAL2* RequestPluginAPI()
	{
		const auto module = GetModuleHandleW(L"ValhallaCombat.dll");
		if (!module) {
			return nullptr;
		}
		const auto request = reinterpret_cast<_RequestPluginAPI>(GetProcAddress(module, "RequestPluginAPI"));
		return request ? static_cast<IVVAL2*>(request(InterfaceVersion::V2)) : nullptr;
	}
}
