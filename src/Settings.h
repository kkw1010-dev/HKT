#pragma once

namespace CIGAR::Settings
{
	// The control panel's choices, kept in Data/SKSE/Plugins/CIGAR.json (a missing or broken file
	// means the defaults: every module on). Safe from any thread.
	void Load();

	bool Enabled(std::string_view a_module);
	// Saves the file. Switching a module off takes its prompts off the screen.
	void SetEnabled(std::string_view a_module, bool a_on);

	// How close the player must stay to the bed or wardrobe last aimed at (Dress).
	inline constexpr float kPlaceRangeMin = 100.0f;
	inline constexpr float kPlaceRangeMax = 600.0f;
	float PlaceRange();
	void SetPlaceRange(float a_range);
	// The keyboard keys (DirectInput scan codes) of CIGAR's prompt key slots. SkyPrompt allows at most
	// four prompts per client at once; each prompt on screen takes the lowest free slot. The default
	// is SkyPrompt's own default, 1-4. Gamepads keep SkyPrompt's default buttons.
	inline constexpr std::size_t kPromptKeyCount = 4;
	using PromptKeyArray = std::array<std::uint32_t, kPromptKeyCount>;
	inline constexpr PromptKeyArray kDefaultPromptKeys{ 2, 3, 4, 5 };
	PromptKeyArray PromptKeys();
	// Saves the file and takes every prompt off the screen, so each is offered again with its new key.
	void SetPromptKey(std::size_t a_slot, std::uint32_t a_key);

	// Prompt-only mode for another mod's own key ("grapple", "surrender", "valhalla"): the module moves that
	// mod's key to a key no keyboard sends (F13/F14), so only CIGAR's prompt triggers it and the
	// real key is free for prompts. ManualKey is the key it had before, restored when switched off.
	// Default: on.
	bool PromptOnly(std::string_view a_target);
	// Saves the file; the owning module applies the change on the game thread.
	void SetPromptOnly(std::string_view a_target, bool a_on);
	std::int32_t ManualKey(std::string_view a_target);
	void SetManualKey(std::string_view a_target, std::int32_t a_key);
	// The same for a target with several keys ("privateneeds": PNO's six hotkeys), one per name.
	std::int32_t ManualKey(std::string_view a_target, std::string_view a_name);
	void SetManualKey(std::string_view a_target, std::string_view a_name, std::int32_t a_key);

	// The Survival Mode hunger stage (1-5) the eat prompt starts at; default 3.
	int EatMinStage();
	void SetEatMinStage(int a_stage);

	// The Private Needs bladder/bowel level (1-5) the needs prompts start at; default 1.
	int NeedsMinStage();
	void SetNeedsMinStage(int a_stage);

	// The enemy distance at which the weapon swap offers a ranged weapon (beyond) or a melee weapon
	// (inside); default 800, the user's choice.
	float WeaponSwapRange();
	void SetWeaponSwapRange(float a_range);

	// How close a guarding humanoid must be for the 유술 prompt; default 250, the user's choice.
	float JujutsuReach();
	void SetJujutsuReach(float a_reach);

	// 유술 tuning, each a fixed value the player can change in the panel (the user's defaults, 2026-09-20):
	// the Valhalla stun share of a 유술 on a guarding target (0.15) and after a perfect parry (0.25), how
	// long a parried attacker stays open to 유술 (1.5 s, at any distance), and the slow motion at the
	// start of the throw (on, x0.3 for 0.5 s).
	struct JujutsuTuning
	{
		float guardStun{ 0.15f };
		float parryStun{ 0.25f };
		float parryWindow{ 1.5f };
		bool slow{ true };
		float slowMultiplier{ 0.3f };
		float slowSeconds{ 0.5f };
	};
	JujutsuTuning JujutsuTune();
	void SetJujutsuTune(const JujutsuTuning& a_tuning);

	// Writes the file; the panel calls this once a slider is released rather than on every drag step.
	void Save();

	// Where Load() read from and whether it succeeded, for the panel's status line.
	std::string SourceDescription();
}
