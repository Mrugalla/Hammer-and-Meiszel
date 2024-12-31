#include "Axiom.h"

namespace dsp
{
	namespace modal
	{
		String toString(StatusMat status)
		{
			switch (status)
			{
			case StatusMat::Processing: return "Processing";
			case StatusMat::UpdatedMaterial: return "UpdatedMaterial";
			case StatusMat::UpdatedProcessor: return "UpdatedProcessor";
			default: return "Unknown";
			}
		}
	}
}