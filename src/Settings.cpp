#include "Settings.h"

#include "Module.h"
#include "Prompt.h"

#include <nlohmann/json.hpp>

#include <map>

namespace CIGAR::Settings
{
	namespace
	{
		// Relative to the game folder, so MO2's VFS maps it to the CIGAR mod folder when the
		// deployed default file is there (otherwise a write lands in MO2's overwrite folder).
		const std::filesystem::path kPath{ L"Data/SKSE/Plugins/CIGAR.json" };
		constexpr float kPlaceRangeDefault = 250.0f;

		std::mutex lock;
		std::map<std::string, bool, std::less<>> enabled;
		float placeRange = kPlaceRangeDefault;
		PromptKeyArray promptKeys = kDefaultPromptKeys;

		struct PromptOnlyState
		{
			bool on{ true };
			std::int32_t manualKey{ -1 };
		};
		constexpr std::array kPromptOnlyTargets{ "grapple"sv, "surrender"sv };
		std::map<std::string, PromptOnlyState, std::less<>> promptOnly;

		void ResetPromptOnly()
		{
			promptOnly.clear();
			for (const auto target : kPromptOnlyTargets) {
				promptOnly.emplace(std::string(target), PromptOnlyState{});
			}
		}
		std::string source = "not loaded";

		void SaveLocked()
		{
			nlohmann::json j;
			for (const auto& [name, on] : enabled) {
				j["modules"][name]["enabled"] = on;
			}
			j["dress"]["placeRange"] = placeRange;
			j["prompt"]["keys"] = promptKeys;
			for (const auto& [target, state] : promptOnly) {
				j["promptOnly"][target]["enabled"] = state.on;
				j["promptOnly"][target]["manualKey"] = state.manualKey;
			}
			std::error_code ec;
			std::filesystem::create_directories(kPath.parent_path(), ec);
			std::ofstream out(kPath, std::ios::binary | std::ios::trunc);
			out << j.dump(2) << '\n';
			out.close();
			if (out) {
				logs::info("settings saved to {}", kPath.string());
			} else {
				logs::error("settings could not be written to {}", kPath.string());
			}
		}
	}

	void Load()
	{
		std::scoped_lock guard(lock);
		enabled.clear();
		for (const auto* module : Modules()) {
			enabled.emplace(module->Name(), true);
		}
		placeRange = kPlaceRangeDefault;
		promptKeys = kDefaultPromptKeys;
		ResetPromptOnly();

		std::ifstream in(kPath, std::ios::binary);
		if (!in) {
			source = "defaults (no CIGAR.json)";
			logs::info("settings: {} not found, using defaults", kPath.string());
			return;
		}
		try {
			const auto j = nlohmann::json::parse(in);
			if (const auto it = j.find("modules"); it != j.end() && it->is_object()) {
				for (auto& [name, on] : enabled) {
					if (const auto m = it->find(name); m != it->end()) {
						on = m->value("enabled", true);
					}
				}
			}
			if (const auto it = j.find("dress"); it != j.end() && it->is_object()) {
				placeRange = std::clamp(it->value("placeRange", kPlaceRangeDefault), kPlaceRangeMin, kPlaceRangeMax);
			}
			if (const auto it = j.find("prompt"); it != j.end() && it->is_object()) {
				if (const auto keys = it->find("keys"); keys != it->end() && keys->is_array()) {
					for (std::size_t i = 0; i < kPromptKeyCount && i < keys->size(); ++i) {
						const auto& v = (*keys)[i];
						// Keyboard scan codes only; anything else keeps that slot's default.
						if (v.is_number_unsigned() && v.get<std::uint32_t>() > 0 && v.get<std::uint32_t>() < 256) {
							promptKeys[i] = v.get<std::uint32_t>();
						} else {
							logs::warn("settings: prompt key {} is not a keyboard key ({}); using {}", i + 1, v.dump(), promptKeys[i]);
						}
					}
				}
			}
			if (const auto it = j.find("promptOnly"); it != j.end() && it->is_object()) {
				for (auto& [target, state] : promptOnly) {
					if (const auto t = it->find(target); t != it->end() && t->is_object()) {
						state.on = t->value("enabled", true);
						state.manualKey = t->value("manualKey", -1);
					}
				}
			}
			source = "CIGAR.json";
		} catch (const std::exception& e) {
			source = "defaults (CIGAR.json unreadable)";
			logs::error("settings: {} is unreadable ({}); using defaults", kPath.string(), e.what());
			return;
		}
		for (const auto& [name, on] : enabled) {
			logs::info("settings: {} {}", name, on ? "on" : "off");
		}
		logs::info("settings: dress place range {:.0f}", placeRange);
		logs::info("settings: prompt keys {} {} {} {}", promptKeys[0], promptKeys[1], promptKeys[2], promptKeys[3]);
		for (const auto& [target, state] : promptOnly) {
			logs::info("settings: {} prompt-only {} (manual key {})", target, state.on ? "on" : "off", state.manualKey);
		}
	}

	bool Enabled(std::string_view a_module)
	{
		std::scoped_lock guard(lock);
		const auto it = enabled.find(a_module);
		return it == enabled.end() || it->second;
	}

	void SetEnabled(std::string_view a_module, bool a_on)
	{
		{
			std::scoped_lock guard(lock);
			enabled.insert_or_assign(std::string(a_module), a_on);
			SaveLocked();
		}
		logs::info("control panel: {} switched {}", a_module, a_on ? "on" : "off");
		if (a_on) {
			return;
		}
		for (const auto* module : Modules()) {
			if (a_module == module->Name()) {
				SKSE::GetTaskInterface()->AddTask([module] {
					Prompts::WithdrawAll(module);
					const_cast<Module*>(module)->OnDisabled();
				});
			}
		}
	}

	float PlaceRange()
	{
		std::scoped_lock guard(lock);
		return placeRange;
	}

	void SetPlaceRange(float a_range)
	{
		std::scoped_lock guard(lock);
		placeRange = std::clamp(a_range, kPlaceRangeMin, kPlaceRangeMax);
	}

	PromptKeyArray PromptKeys()
	{
		std::scoped_lock guard(lock);
		return promptKeys;
	}

	void SetPromptKey(std::size_t a_slot, std::uint32_t a_key)
	{
		if (a_slot >= kPromptKeyCount || a_key == 0 || a_key >= 256) {
			return;
		}
		{
			std::scoped_lock guard(lock);
			promptKeys[a_slot] = a_key;
			SaveLocked();
		}
		logs::info("control panel: prompt key {} set to {}", a_slot + 1, a_key);
		// SkyPrompt keeps a queued prompt's key, so take every prompt down; each is offered again.
		SKSE::GetTaskInterface()->AddTask([] { Prompts::WithdrawEverything(); });
	}

	bool PromptOnly(std::string_view a_target)
	{
		std::scoped_lock guard(lock);
		const auto it = promptOnly.find(a_target);
		return it == promptOnly.end() || it->second.on;
	}

	void SetPromptOnly(std::string_view a_target, bool a_on)
	{
		std::scoped_lock guard(lock);
		promptOnly[std::string(a_target)].on = a_on;
		SaveLocked();
		logs::info("control panel: {} prompt-only {}", a_target, a_on ? "on" : "off");
	}

	std::int32_t ManualKey(std::string_view a_target)
	{
		std::scoped_lock guard(lock);
		const auto it = promptOnly.find(a_target);
		return it == promptOnly.end() ? -1 : it->second.manualKey;
	}

	void SetManualKey(std::string_view a_target, std::int32_t a_key)
	{
		std::scoped_lock guard(lock);
		auto& state = promptOnly[std::string(a_target)];
		if (state.manualKey == a_key) {
			return;
		}
		state.manualKey = a_key;
		SaveLocked();
		logs::info("settings: {} manual key remembered as {}", a_target, a_key);
	}

	void Save()
	{
		std::scoped_lock guard(lock);
		SaveLocked();
		logs::info("control panel: dress place range {:.0f}", placeRange);
	}

	std::string SourceDescription()
	{
		std::scoped_lock guard(lock);
		return source;
	}
}
