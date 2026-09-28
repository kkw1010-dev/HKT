#pragma once

// Log-only probe for the NPC push-through plan (docs/038), author build only. Nothing here offers a
// prompt or writes an engine value: it performs the vanilla bump on the NPC under the crosshair, or
// watches the bumps the game makes by itself, and logs what follows ([PushProbe] lines in CIGAR.log).
namespace CIGAR::PushProbe
{
	enum class Kind
	{
		kAction,  // ActionBumpedInto (default object 90) through TESActionData; the engine picks the idle
		kFront,   // the graph events of the four IDLE children of BumpedIntoRoot, sent directly
		kBack,
		kLeft,
		kRight
	};

	// Any thread: arms the probe; it fires on the NPC under the crosshair at the next unpaused tick,
	// so the panel can be closed first and the measurement is not paused with the menu.
	void Arm(Kind a_kind);
	// Any thread: logs every bump the game gives an NPC near the player (walking or sprinting into
	// one), with the same before/after measurement.
	void SetWatch(bool a_on);
	bool Watching();
	// Stage 0b (docs/038): the push clip candidates of tools/push_test_assets.py ("CIGAR Push Test"),
	// played on the player through Offset Movement Animation. Each is keyed on its own
	// iGPMAAnimationType; iGPMAOffsetType picks the arm (0 both arms and upper body, 1 right, 2 left).
	// Stage 0c adds 3805 cut short and a value-0 entry that picks 3805 or 3806 by where the NPC stands.
	struct Gesture
	{
		int value;
		int offsetType;
		float seconds;
		const char* label;
	};
	std::span<const Gesture> Gestures();
	// Any thread: arms one; it plays as soon as the player moves (within 20 s), and stops after its
	// length, both graph variables back at 0.
	void ArmGesture(std::size_t a_index);

	// kDataLoaded: the dialogue event sink.
	void RegisterEvents();
	// Game thread, every 100 ms while the game runs.
	void Tick();
	std::string Status();
}
