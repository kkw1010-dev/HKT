#pragma once

namespace CIGAR
{
	// What the control panel shows for one module (Panel.cpp's table, and Personal::FindLabel).
	struct ModuleLabel
	{
		std::string_view module;
		const char* titleKo;
		const char* titleEn;
		// The mod it waits for (a name, the same in both languages); empty for none.
		const char* needs;
		// What the player gets from it, in one or two sentences.
		const char* whatKo;
		const char* whatEn;
	};
}
