#include "TDMLock.h"

#include "Util.h"

#include "TDM/TrueDirectionalMovementAPI.h"

namespace CIGAR::TDMLock
{
	namespace
	{
		// TDM reads its MCM Helper settings over the shipped defaults; so does this.
		constexpr auto kSettings = "Data/MCM/Settings/TrueDirectionalMovement.ini"sv;
		constexpr auto kDefaults = "Data/MCM/Config/TrueDirectionalMovement/settings.ini"sv;
		constexpr std::int64_t kDefaultKey = 258;  // middle mouse button

		TDM_API::IVTDM1* api{ nullptr };
		std::int64_t key{ -1 };
		std::string_view source{ "not resolved" };
		bool busy{ false };
	}

	void Resolve()
	{
		busy = false;
		if (!api) {
			api = TDM_API::RequestPluginAPI();
		}
		if (!api) {
			key = -1;
			source = "TDM absent";
			return;
		}
		source = kSettings;
		auto found = Util::IniInt(std::filesystem::path{ kSettings }, "Keys", "uTargetLockKey");
		if (!found) {
			source = kDefaults;
			found = Util::IniInt(std::filesystem::path{ kDefaults }, "Keys", "uTargetLockKey");
		}
		if (!found) {
			source = "TDM default";
			found = kDefaultKey;
		}
		key = *found;
	}

	TDM_API::IVTDM1* Api()
	{
		return api;
	}

	bool Locked()
	{
		return api && api->GetTargetLockState();
	}

	std::int64_t Key()
	{
		return key;
	}

	std::string_view KeySource()
	{
		return source;
	}

	bool Pressable()
	{
		return key >= 0 && key < 264;
	}

	bool Press()
	{
		return Util::PressKey(key);
	}

	void SetBusy(bool a_busy)
	{
		busy = a_busy;
	}

	bool Busy()
	{
		return busy;
	}
}
