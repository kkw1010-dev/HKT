#include "Settings.h"

#include "Eat.h"
#include "Module.h"
#include "Jujutsu.h"
#include "Needs.h"
#include "Prompt.h"
#include "WeaponSwap.h"

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
		int eatMinStage = Eat::kMinStageDefault;
		int needsMinStage = Needs::kMinStageDefault;
		float swapRange = WeaponSwap::kRangeDefault;
		float jujutsuReach = Jujutsu::kReachDefault;
		JujutsuTuning jujutsuTuning;

		JujutsuTuning Clamped(JujutsuTuning a_t)
		{
			a_t.guardStun = std::clamp(a_t.guardStun, 0.0f, 1.0f);
			a_t.parryStun = std::clamp(a_t.parryStun, 0.0f, 1.0f);
			a_t.parryWindow = std::clamp(a_t.parryWindow, 0.5f, 5.0f);
			a_t.slowMultiplier = std::clamp(a_t.slowMultiplier, 0.1f, 1.0f);
			a_t.slowSeconds = std::clamp(a_t.slowSeconds, 0.1f, 3.0f);
			return a_t;
		}

		struct PromptOnlyState
		{
			bool on{ true };
			std::int32_t manualKey{ -1 };
			// Targets with several keys keep them by name.
			std::map<std::string, std::int32_t, std::less<>> manualKeys;
		};
		constexpr std::array kPromptOnlyTargets{ "grapple"sv, "surrender"sv, "valhalla"sv, "privateneeds"sv };
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
			j["eat"]["minStage"] = eatMinStage;
			j["needs"]["minStage"] = needsMinStage;
			j["weaponSwap"]["range"] = swapRange;
			j["jujutsu"]["reach"] = jujutsuReach;
			j["jujutsu"]["guardStun"] = jujutsuTuning.guardStun;
			j["jujutsu"]["parryStun"] = jujutsuTuning.parryStun;
			j["jujutsu"]["parryWindow"] = jujutsuTuning.parryWindow;
			j["jujutsu"]["slow"] = jujutsuTuning.slow;
			j["jujutsu"]["slowMultiplier"] = jujutsuTuning.slowMultiplier;
			j["jujutsu"]["slowSeconds"] = jujutsuTuning.slowSeconds;
			for (const auto& [target, state] : promptOnly) {
				j["promptOnly"][target]["enabled"] = state.on;
				j["promptOnly"][target]["manualKey"] = state.manualKey;
				for (const auto& [name, key] : state.manualKeys) {
					j["promptOnly"][target]["manualKeys"][name] = key;
				}
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
		eatMinStage = Eat::kMinStageDefault;
		needsMinStage = Needs::kMinStageDefault;
		swapRange = WeaponSwap::kRangeDefault;
		jujutsuReach = Jujutsu::kReachDefault;
		jujutsuTuning = {};
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
			if (const auto it = j.find("eat"); it != j.end() && it->is_object()) {
				eatMinStage = std::clamp(it->value("minStage", Eat::kMinStageDefault), Eat::kMinStageLow, Eat::kMinStageHigh);
			}
			if (const auto it = j.find("needs"); it != j.end() && it->is_object()) {
				needsMinStage = std::clamp(it->value("minStage", Needs::kMinStageDefault), Needs::kMinStageLow, Needs::kMinStageHigh);
			}
			if (const auto it = j.find("jujutsu"); it != j.end() && it->is_object()) {
				jujutsuReach = std::clamp(it->value("reach", Jujutsu::kReachDefault), Jujutsu::kReachLow, Jujutsu::kReachHigh);
				const JujutsuTuning d;
				jujutsuTuning = Clamped({ it->value("guardStun", d.guardStun), it->value("parryStun", d.parryStun),
					it->value("parryWindow", d.parryWindow), it->value("slow", d.slow), it->value("slowMultiplier", d.slowMultiplier),
					it->value("slowSeconds", d.slowSeconds) });
			}
			if (const auto it = j.find("weaponSwap"); it != j.end() && it->is_object()) {
				swapRange = std::clamp(it->value("range", WeaponSwap::kRangeDefault), WeaponSwap::kRangeLow, WeaponSwap::kRangeHigh);
			}
			if (const auto it = j.find("promptOnly"); it != j.end() && it->is_object()) {
				for (auto& [target, state] : promptOnly) {
					if (const auto t = it->find(target); t != it->end() && t->is_object()) {
						state.on = t->value("enabled", true);
						state.manualKey = t->value("manualKey", -1);
						if (const auto keys = t->find("manualKeys"); keys != t->end() && keys->is_object()) {
							for (const auto& [name, key] : keys->items()) {
								if (key.is_number_integer()) {
									state.manualKeys[name] = key.get<std::int32_t>();
								}
							}
						}
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
			for (const auto& [name, key] : state.manualKeys) {
				logs::info("settings: {} manual key {} = {}", target, name, key);
			}
		}
		logs::info("settings: eat from hunger stage {}", eatMinStage);
		logs::info("settings: needs from level {}", needsMinStage);
		logs::info("settings: weapon swap range {:.0f}", swapRange);
		logs::info("settings: jujutsu reach {:.0f}", jujutsuReach);
		logs::info("settings: jujutsu stun guard {:.2f} parry {:.2f}, parry window {:.1f} s, slow {} x{:.2f} for {:.1f} s",
			jujutsuTuning.guardStun, jujutsuTuning.parryStun, jujutsuTuning.parryWindow, jujutsuTuning.slow, jujutsuTuning.slowMultiplier,
			jujutsuTuning.slowSeconds);
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

	int EatMinStage()
	{
		std::scoped_lock guard(lock);
		return eatMinStage;
	}

	void SetEatMinStage(int a_stage)
	{
		std::scoped_lock guard(lock);
		eatMinStage = std::clamp(a_stage, Eat::kMinStageLow, Eat::kMinStageHigh);
	}

	int NeedsMinStage()
	{
		std::scoped_lock guard(lock);
		return needsMinStage;
	}

	void SetNeedsMinStage(int a_stage)
	{
		std::scoped_lock guard(lock);
		needsMinStage = std::clamp(a_stage, Needs::kMinStageLow, Needs::kMinStageHigh);
	}

	float JujutsuReach()
	{
		std::scoped_lock guard(lock);
		return jujutsuReach;
	}

	void SetJujutsuReach(float a_reach)
	{
		std::scoped_lock guard(lock);
		jujutsuReach = std::clamp(a_reach, Jujutsu::kReachLow, Jujutsu::kReachHigh);
	}

	JujutsuTuning JujutsuTune()
	{
		std::scoped_lock guard(lock);
		return jujutsuTuning;
	}

	void SetJujutsuTune(const JujutsuTuning& a_tuning)
	{
		std::scoped_lock guard(lock);
		jujutsuTuning = Clamped(a_tuning);
	}

	float WeaponSwapRange()
	{
		std::scoped_lock guard(lock);
		return swapRange;
	}

	void SetWeaponSwapRange(float a_range)
	{
		std::scoped_lock guard(lock);
		swapRange = std::clamp(a_range, WeaponSwap::kRangeLow, WeaponSwap::kRangeHigh);
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

	std::int32_t ManualKey(std::string_view a_target, std::string_view a_name)
	{
		std::scoped_lock guard(lock);
		const auto it = promptOnly.find(a_target);
		if (it == promptOnly.end()) {
			return -1;
		}
		const auto key = it->second.manualKeys.find(a_name);
		return key == it->second.manualKeys.end() ? -1 : key->second;
	}

	void SetManualKey(std::string_view a_target, std::string_view a_name, std::int32_t a_key)
	{
		std::scoped_lock guard(lock);
		auto& keys = promptOnly[std::string(a_target)].manualKeys;
		const auto it = keys.find(a_name);
		if ((it == keys.end() && a_key < 0) || (it != keys.end() && it->second == a_key)) {
			return;
		}
		if (a_key < 0) {
			keys.erase(it);
		} else {
			keys.insert_or_assign(std::string(a_name), a_key);
		}
		SaveLocked();
		logs::info("settings: {} manual key {} remembered as {}", a_target, a_name, a_key);
	}

	void Save()
	{
		std::scoped_lock guard(lock);
		SaveLocked();
		logs::info("control panel: dress place range {:.0f}, eat from hunger stage {}, needs from level {}, weapon swap range {:.0f}, jujutsu reach {:.0f}", placeRange, eatMinStage, needsMinStage, swapRange, jujutsuReach);
	}

	std::string SourceDescription()
	{
		std::scoped_lock guard(lock);
		return source;
	}
}
