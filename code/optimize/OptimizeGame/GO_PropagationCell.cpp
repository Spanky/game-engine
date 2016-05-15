#include "StableHeaders.h"
#include "GO_PropagationCell.h"

namespace GO
{
	PropagationCell::PropagationCell()
		: myValue(0.0f)
	{
	}

	void PropagationCell::update()
	{
		myValue -= 0.01f;
	}
}