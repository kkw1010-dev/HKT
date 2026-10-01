#pragma once

#include "Module.h"

namespace CIGAR
{
	// Parts of Rest with their own switch in the control panel: pass time and leaning (3.1.1, asked for on
	// Nexus), sitting and lying (after r13: what players want is to keep leaning and switch those two off).
	// They have no prompts of their own; Rest asks Settings whether each is on. Rest itself must be on for
	// any of them to do anything.
	class RestPart final : public Module
	{
	public:
		static RestPart* PassTime();
		static RestPart* Lean();
		static RestPart* Sit();
		static RestPart* Lie();

		const char* Name() const override { return name; }
		void OnGameLoaded() override {}
		void Tick() override {}
		void OnAccepted(std::uint16_t) override {}
		// Switched off while in use: Rest ends a held pass time; a sit, lie or lean prompt goes on Rest's next tick.
		void OnDisabled() override;

	private:
		explicit RestPart(const char* a_name) :
			name(a_name) {}

		const char* name;
	};
}
