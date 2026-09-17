#pragma once

namespace CIGAR::Panel
{
	// Adds the CIGAR page to SKSE Menu Framework when it is loaded; otherwise logs why and returns.
	// Call after every SKSE plugin has loaded (kPostLoad).
	void Register();
}
