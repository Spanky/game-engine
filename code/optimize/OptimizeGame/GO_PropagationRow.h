#pragma once

#include "GO_PropagationCell.h"

namespace GO
{
	class PropagationRow
	{
	public:
		static const int MatrixDimensions = 501;

		//PropagationRow(size_t aColumnCount);
		//PropagationRow();

		//std::vector<PropagationCell> myCells;
		PropagationCell myCells[MatrixDimensions];
	};
}