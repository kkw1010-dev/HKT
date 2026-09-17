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
	// Writes the file; the panel calls this once a slider is released rather than on every drag step.
	void Save();

	// Where Load() read from and whether it succeeded, for the panel's status line.
	std::string SourceDescription();
}
