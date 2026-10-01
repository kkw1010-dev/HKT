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
	// The keyboard or mouse keys of CIGAR's prompt key slots: DirectInput scan codes, or SkyPrompt's
	// mouse codes for the middle and side buttons (258 middle, 259 and up the side buttons; 256 and 257
	// are attack and block and stay out, the user's choice of 2026-09-28). SkyPrompt allows at most
	// four prompts per client at once; each prompt on screen takes the lowest free slot. The default
	// is SkyPrompt's own default, 1-4.
	inline constexpr std::size_t kPromptKeyCount = 4;
	using PromptKeyArray = std::array<std::uint32_t, kPromptKeyCount>;
	inline constexpr PromptKeyArray kDefaultPromptKeys{ 2, 3, 4, 5 };
	inline constexpr std::uint32_t kMouseFirst = 258;
	inline constexpr std::uint32_t kMouseLast = 263;
	constexpr bool IsPromptKey(std::uint32_t a_code)
	{
		return (a_code > 0 && a_code < 256) || (a_code >= kMouseFirst && a_code <= kMouseLast);
	}
	PromptKeyArray PromptKeys();
	// Saves the file and takes every prompt off the screen, so each is offered again with its new key.
	void SetPromptKey(std::size_t a_slot, std::uint32_t a_key);

	// The gamepad buttons of the slots (the user, 2026-09-28; docs/016): SkyPrompt's own slot buttons
	// (A/B/X/Y by default, or what the player set in SkyPrompt's Controls), or the D-pad with slot 1 Up,
	// 2 Down, 3 Left, 4 Right. Up and Down come first because the lowest free slot is taken first and
	// Left and Right are also SkyPrompt's paging buttons.
	enum class PadPreset
	{
		kSkyPrompt,
		kDpad
	};
	// SKSE's linear gamepad codes (the ones SkyPrompt's settings store): 266 Up, 267 Down, 268 Left, 269 Right.
	inline constexpr PromptKeyArray kDpadButtons{ 266, 267, 268, 269 };
	PadPreset PadButtons();
	// Saves the file and takes every prompt off the screen, so each is offered again with its new button.
	void SetPadButtons(PadPreset a_preset);

	// Prompt-only mode for another mod's own key ("grapple", "surrender", "valhalla", "fillherup"): the module moves that
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

	// The Private Needs bladder/bowel fill (percent) the needs prompts start at; default 50, the user's choice.
	int NeedsMinPercent();
	void SetNeedsMinPercent(int a_percent);

	// The enemy distance at which the weapon swap offers a ranged weapon (beyond) or a melee weapon
	// (inside); default 800, the user's choice.
	float WeaponSwapRange();
	void SetWeaponSwapRange(float a_range);

	// How close a guarding humanoid must be for the 유술 prompt; default 250, the user's choice.
	float JujutsuReach();
	void SetJujutsuReach(float a_reach);

	// 유술 tuning the player can change in the panel, applied at the victim's kill moment:
	// - guardStun: the Valhalla stun share (with Valhalla Combat); default 0.15, the user's choice
	//   (2026-09-20).
	// - staminaDamage: without Valhalla, the share of the victim's maximum stamina taken; default 1.0,
	//   which is what 유술 always did (all of it).
	// - healthDamage: the share of the victim's maximum health taken, never leaving less than 1
	//   health; default 0.05, the value 유술 always used.
	// The two damage values were made adjustable at the user's request (2026-09-26, Nexus edition).
	struct JujutsuTuning
	{
		float guardStun{ 0.15f };
		float staminaDamage{ 1.0f };
		float healthDamage{ 0.05f };
	};
	JujutsuTuning JujutsuTune();
	void SetJujutsuTune(const JujutsuTuning& a_tuning);

	// Which potions the Potion module offers and when. The defaults are the user's values.
	// Each threshold is a fraction of the full bar.
	struct PotionTuning
	{
		bool health{ true };
		bool stamina{ true };
		bool magicka{ true };
		bool curePoison{ true };
		bool cureDisease{ true };
		bool waterBreathing{ true };
		float healthThreshold{ 0.5f };
		// At or below this, the strongest healing potion is picked instead of the weakest that
		// covers the missing health.
		float urgentHealthThreshold{ 0.2f };
		float staminaThreshold{ 0.5f };
		float magickaThreshold{ 0.5f };
	};
	PotionTuning PotionTune();
	void SetPotionTune(const PotionTuning& a_tuning);

	// The game speed pass time climbs to while its key is held (Rest). The user chose the steps,
	// off (1) / 1.5 / 2 / 2.5 / 3, and adopted 3 as the default (2026-09-22).
	inline constexpr std::array kRestGameSpeedSteps{ 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
	inline constexpr float kRestGameSpeedDefault = 3.0f;
	float RestGameSpeed();
	// Snaps to the nearest step.
	void SetRestGameSpeed(float a_speed);

	// Third person: how far to the camera's right of the point ahead of the head the prompts sit
	// (PromptAnchor). The user kept the N7 candidates 0 / 10 / 15 / 20 / 30 as the player's choice,
	// default 15 (2026-09-27).
	// 3.1.1: a wide range on purpose, negative for the left (the user, 2026-10-01: a Nexus comment asked to
	// hide prompts during Observe; the answer is a wider choice of positions instead, and a position that
	// leaves the screen is the player's choice; nothing pulls it back, except at the default offset, where
	// PromptAnchor keeps the mark on screen). The first steps were 0-30.
	inline constexpr std::array kPromptRightSteps{ -200.0f, -150.0f, -100.0f, -60.0f, -30.0f, -15.0f, 0.0f, 10.0f, 15.0f,
		20.0f, 30.0f, 45.0f, 60.0f, 80.0f, 100.0f, 150.0f, 200.0f };
	inline constexpr float kPromptRightDefault = 15.0f;
	float PromptRight();
	// Snaps to the nearest step and moves the marker from the next frame.
	void SetPromptRight(float a_units);

	// Writes the file; the panel calls this once a slider is released rather than on every drag step.
	void Save();

	// Where Load() read from and whether it succeeded, for the panel's status line.
	std::string SourceDescription();

	// The player-facing language: "auto" (default; Text::Resolve reads the game's own text), "ko" or
	// "en". SetLanguageChoice saves the file; the caller re-resolves.
	std::string LanguageChoice();
	void SetLanguageChoice(std::string_view a_choice);
}
