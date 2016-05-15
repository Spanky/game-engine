#pragma once

#include "GO_PropagationRow.h"

namespace GO
{
	class PropagationMatrix
	{
	public:
		PropagationMatrix(int aRowCount, int aColumnCount, float aCellHalfSize);

		void setCenter(sf::Vector2f aCenterPoint);
		void resetGrid(sf::Vector2f aCenterPoint);

		sf::Vector2f mapGridCoordinateToPosition(sf::Vector2i aGridCoordinate) const;

		sf::Vector2i mapPositionToGridCoordinates(sf::Vector2f aPosition) const;
		sf::Vector2i mapPositionToUnconstrainedGridCoordinates(sf::Vector2f aPosition) const;

		void addContribution(sf::Vector2f aContributionLocation);
		void updateCells();

		void debugDraw();

	private:
		void clearColumns(int aStartColumn, int anEndColumn);
		void clearRows(int aStartRow, int anEndRow);

		bool isLocatedOnGrid(sf::Vector2i aGridCoordinate) const;

		float myCellHalfSize;
		sf::Vector2f myCenter;
		sf::Vector2i myDimensions;
		sf::Vector2i myCenterCell;

		PropagationRow myRows[PropagationRow::MatrixDimensions];
	};
}