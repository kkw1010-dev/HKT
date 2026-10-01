#include "RestParts.h"

#include "Rest.h"

namespace CIGAR
{
	RestPart* RestPart::PassTime()
	{
		static RestPart part("PassTime");
		return &part;
	}

	RestPart* RestPart::Lean()
	{
		static RestPart part("Lean");
		return &part;
	}

	void RestPart::OnDisabled()
	{
		if (this == PassTime()) {
			Rest::GetSingleton()->PassTimeSwitchedOff();
		}
		Log("switched off");
	}
}
