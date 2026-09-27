#pragma once

#include "Module.h"
#include "ModuleLabel.h"

// Personal modules: local-only additions for the author's own game, never published. CMake compiles
// the sources of the sibling folder ../CIGAR-Personal/src into the author build when it exists and
// defines CIGAR_PERSONAL; a release build (CIGAR_RELEASE) never includes them. That folder is its own
// git repository with no remote.
namespace CIGAR::Personal
{
	// Appended to Modules(): registered, ticked, saved and switched like any other module.
	std::span<Module* const> Modules();
	// The panel text of a personal module, or nullptr.
	const ModuleLabel* FindLabel(std::string_view a_module);
	// A string every personal build carries: tools/make_release.py refuses a DLL that has it, and
	// tools/verify_deploy.py expects it in the author build when the folder has sources.
	const char* BuildMarker();
}
