#pragma once

#include "Module.h"

namespace CIGAR
{
	// Two parts of Rest with their own switch in the control panel (3.1.1, asked for on Nexus): pass time
	// and leaning. They have no prompts of their own; Rest asks Settings whether each is on. Rest itself
	// must be on for either to do anything.
	class RestPart final : public Module
	{
	public:
		static RestPart* PassTime();
		static RestPart* Lean();

		const char* Name() const override { return name; }
		void OnGameLoaded() override {}
		void Tick() override {}
		void OnAccepted(std::uint16_t) override {}
		// Switched off while in use: Rest ends a held pass time; a lean prompt goes on Rest's next tick.
		void OnDisabled() override;

	private:
		explicit RestPart(const char* a_name) :
			name(a_name) {}

		const char* name;
	};
}
