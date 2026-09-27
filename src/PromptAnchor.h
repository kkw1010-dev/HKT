#pragma once

// Where SkyPrompt draws CIGAR's prompts (the user, 2026-09-27; docs/007 "Prompt placement").
//
// SkyPrompt places a prompt by the reference it is attached to, and API 2.0 has no offset. On an
// actor it draws beside the head; on any other reference it draws above that reference's bounding
// box. So in third person CIGAR attaches its prompts to one invisible marker (a disabled XMarker
// placed once per save and kept in the co-save) and moves it every frame to a point ahead of the
// head and a little to the right. In first person, or whenever the marker cannot be used, prompts
// attach to the player as before. N6 passed on 2026-09-27; the right offset is still being chosen (N7).
namespace CIGAR::PromptAnchor
{
	// kDataLoaded: hooks PlayerCharacter::Update for the per-frame move.
	void Install();
	// Game thread, after every load: checks the co-saved marker or places a new one, and checks that
	// SkyPrompt will be able to read its bounds.
	void OnGameLoaded();
	// Game thread, about ten times a second while the game runs: marker recovery and the check that
	// the per-frame hook is still called.
	void Tick();
	void Save(SKSE::SerializationInterface* a_intfc);
	void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version);
	void Revert();

	// The reference a prompt offered now should attach to: the marker, or the player (0x14).
	RE::FormID RefID();

	// Units ahead of the head, along the direction the character faces: the user's choice after N6.
	inline constexpr float kForward = 40.0f;
	// Units to the camera's right. Chosen in N7 from these candidates (author panel); 15 until then.
	inline constexpr std::array kRightCandidates{ 0.0f, 10.0f, 15.0f, 20.0f, 30.0f };
	inline constexpr float kRightDefault = 15.0f;
	float Right();
	void SetRight(float a_units);
	std::string Status();
}
