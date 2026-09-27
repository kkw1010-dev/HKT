#pragma once

// Where SkyPrompt draws CIGAR's prompts (the user, 2026-09-27; docs/007 "Prompt placement").
//
// SkyPrompt places a prompt by the reference it is attached to, and API 2.0 has no offset. On an
// actor it draws beside the head; on any other reference it draws above that reference's bounding
// box. So CIGAR attaches its prompts to one invisible marker (a disabled XMarker it places once
// and keeps in the co-save) and moves the marker every frame to a point ahead of the head, in the
// direction the character faces. In first person, or without a marker, prompts attach to the
// player as before.
//
// Probe stage: the forward distance is one of a few candidates switched from the author panel.
namespace CIGAR::PromptAnchor
{
	// kDataLoaded: hooks PlayerCharacter::Update for the per-frame move.
	void Install();
	// Game thread, after every load: finds the co-saved marker or places a new one, and checks
	// that SkyPrompt will be able to read its bounds.
	void OnGameLoaded();
	void Save(SKSE::SerializationInterface* a_intfc);
	void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version);
	void Revert();

	// The reference a prompt offered now should attach to: the marker, or the player (0x14).
	RE::FormID RefID();

	// Author panel (probe): forward distance in units; 0 attaches to the player as before.
	inline constexpr std::array kCandidates{ 0.0f, 25.0f, 40.0f, 60.0f };
	float Distance();
	void SetDistance(float a_units);
	std::string Status();
}
