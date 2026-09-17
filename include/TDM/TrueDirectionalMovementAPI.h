#pragma once

// True Directional Movement modder interface, V1 part only.
// Trimmed from ersh1/TrueDirectionalMovement src/TrueDirectionalMovementAPI.h @ 57b913a. Later
// interface versions only append virtuals, so the V1 layout holds for the V5 singleton the DLL
// returns. RequestPluginAPI uses GetModuleHandleW because CIGAR builds with UNICODE.

namespace TDM_API
{
	enum class InterfaceVersion : std::uint8_t
	{
		V1,
		V2,
		V3,
		V4,
		V5
	};

	enum class APIResult : std::uint8_t
	{
		OK,
		NotOwner,
		MustKeep,
		AlreadyGiven,
		AlreadyTaken,
		BadThread,
	};

	class IVTDM1
	{
	public:
		[[nodiscard]] virtual unsigned long GetTDMThreadId() const noexcept = 0;
		[[nodiscard]] virtual bool GetDirectionalMovementState() const noexcept = 0;
		[[nodiscard]] virtual bool GetTargetLockState() const noexcept = 0;
		[[nodiscard]] virtual RE::ActorHandle GetCurrentTarget() const noexcept = 0;
		[[nodiscard]] virtual APIResult RequestDisableDirectionalMovement(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;
		[[nodiscard]] virtual APIResult RequestDisableHeadtracking(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;
		virtual SKSE::PluginHandle GetDisableDirectionalMovementOwner() const noexcept = 0;
		virtual SKSE::PluginHandle GetDisableHeadtrackingOwner() const noexcept = 0;
		virtual APIResult ReleaseDisableDirectionalMovement(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;
		virtual APIResult ReleaseDisableHeadtracking(SKSE::PluginHandle a_myPluginHandle) noexcept = 0;
	};

	using _RequestPluginAPI = void* (*)(const InterfaceVersion interfaceVersion);

	// Call at or after kPostLoad.
	[[nodiscard]] inline IVTDM1* RequestPluginAPI()
	{
		const auto module = GetModuleHandleW(L"TrueDirectionalMovement.dll");
		if (!module) {
			return nullptr;
		}
		const auto request = reinterpret_cast<_RequestPluginAPI>(GetProcAddress(module, "RequestPluginAPI"));
		return request ? static_cast<IVTDM1*>(request(InterfaceVersion::V1)) : nullptr;
	}
}
