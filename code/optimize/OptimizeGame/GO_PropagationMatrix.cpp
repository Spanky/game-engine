#include "StableHeaders.h"
#include "GO_PropagationMatrix.h"

#include "GO_GameInstance.h"
#include "GO_HacksGlobalResources.h"

namespace
{
	static int Wrap(int kX, int const kLowerBound, int const kUpperBound)
	{
		int range_size = kUpperBound - kLowerBound + 1;

		if (kX < kLowerBound)
			kX += range_size * ((kLowerBound - kX) / range_size + 1);

		return kLowerBound + (kX - kLowerBound) % range_size;
	}
}

namespace GO
{
	PropagationMatrix::PropagationMatrix(int aRowCount, int aColumnCount, float aCellHalfSize)
		//: myRows(aRowCount, PropagationRow(aColumnCount))
		: myCellHalfSize(aCellHalfSize)
		, myDimensions(aRowCount, aColumnCount)
	{
		GO_ASSERT(aRowCount % 2 == 1, "Propagation requires an odd number of rows");
		GO_ASSERT(aColumnCount % 2 == 1, "Propagation requires an odd number of columns");
		GO_ASSERT(aRowCount >= 3, "Propagation requires at least 3 cells wide");

		GO_ASSERT(aRowCount == PropagationRow::MatrixDimensions, "Propagation is hard-coded to 3 rows right now");
		GO_ASSERT(aColumnCount == PropagationRow::MatrixDimensions, "Propagation is hard-coded to 3 columns right now");
	}

	void PropagationMatrix::addContribution(sf::Vector2f aContributionLocation)
	{
		sf::Vector2i gridCoordinates = mapPositionToUnconstrainedGridCoordinates(aContributionLocation);
		if(isLocatedOnGrid(gridCoordinates))
		{
			myRows[gridCoordinates.y].myCells[gridCoordinates.x].myValue += 1.0f;
		}
	}

	bool PropagationMatrix::isLocatedOnGrid(sf::Vector2i aGridCoordinate) const
	{
		return (aGridCoordinate.x >= 0 && aGridCoordinate.x < myDimensions.x && aGridCoordinate.y >= 0 && aGridCoordinate.y < myDimensions.y);
	}

	void PropagationMatrix::setCenter(sf::Vector2f aCenterPoint)
	{
		sf::Vector2i newCenterInExistingGrid = mapPositionToUnconstrainedGridCoordinates(aCenterPoint);
		sf::Vector2i existingCenterInCurrentGrid = mapPositionToGridCoordinates(myCenter);

		sf::Vector2i distanceBetweenGrids = newCenterInExistingGrid - existingCenterInCurrentGrid;
		
		// The distance between the current grid and the 'new' grid is larger than our capacity so there
		// is no way that anything can overlap. We can just toss the entire grid and start a new one
		if((std::abs(distanceBetweenGrids.x) >= myDimensions.x) || (std::abs(distanceBetweenGrids.y) >= myDimensions.y))
		{
			resetGrid(aCenterPoint);
		}
		else // There is some overlap
		{
			myCenterCell += distanceBetweenGrids;
			myCenterCell.x = Wrap(myCenterCell.x, 0, myDimensions.x - 1);
			myCenterCell.y = Wrap(myCenterCell.y, 0, myDimensions.y - 1);

			// If we are moving to the right, we clear out (distance.x) columns from the left side of the grid
			if(distanceBetweenGrids.x > 0)
			{
				// Start at the center and find the left edge of the new grid. We need to clear the 'distance.x' cells
				// before that new edge since they are now too far to the left of our grid to matter (they become
				// the new 'right side' columns that are now present in the grid)
				
				// We will clear all the way up the column right before our 'new left edge'
				int endClearColumn = Wrap(myCenterCell.x - ((myDimensions.x - 1) / 2) - 1, 0, myDimensions.x - 1);
				int startClearColumn = Wrap(endClearColumn - distanceBetweenGrids.x + 1, 0, myDimensions.x - 1);
				clearColumns(startClearColumn, endClearColumn);
			}
			else if(distanceBetweenGrids.x < 0)
			{
				// We will clear starting at the column right after our 'new right edge'
				int startClearColumn = Wrap(myCenterCell.x + ((myDimensions.x - 1) / 2) + 1, 0, myDimensions.x - 1);
				int endClearColumn = Wrap(startClearColumn - distanceBetweenGrids.x - 1, 0, myDimensions.x - 1);
				clearColumns(startClearColumn, endClearColumn);
			}

			if(distanceBetweenGrids.y > 0) // Moving down
			{
				// We will clear all the way up to the row right before our 'new top edge'
				int endClearRow = Wrap(myCenterCell.y - ((myDimensions.y - 1) / 2) - 1, 0, myDimensions.y - 1);
				int startClearRow = Wrap(endClearRow - distanceBetweenGrids.y + 1, 0, myDimensions.x - 1);
				clearColumns(startClearRow, endClearRow);
			}
			else if(distanceBetweenGrids.y < 0) // Moving up
			{
				// We will clear starting at the row right after our 'new bottom edge'
				int startClearRow = Wrap(myCenterCell.y + ((myDimensions.y - 1) / 2) + 1, 0, myDimensions.y - 1);
				int endClearRow = Wrap(startClearRow - distanceBetweenGrids.y - 1, 0, myDimensions.y - 1);
				clearColumns(startClearRow, endClearRow);
			}
		}

		myCenter = aCenterPoint;
	}

	void PropagationMatrix::clearColumns(int aStartColumn, int anEndColumn)
	{
		GO_ASSERT(aStartColumn >= 0 && aStartColumn < myDimensions.x, "Starting column for clear is outside of grid range");
		GO_ASSERT(anEndColumn >= 0 && anEndColumn < myDimensions.x, "Ending column for clear is outside of grid range");


		// Traverse each row and clear all columns within that row
		for (int currentRow = 0; currentRow < myDimensions.y; currentRow++)
		{
			GO_ASSERT(currentRow < PropagationRow::MatrixDimensions, "");
			PropagationRow& row = myRows[currentRow];
			const int numColumnsToClear = Wrap(anEndColumn - aStartColumn + 1, 0, myDimensions.x);
			for (int currentColumn = aStartColumn; currentColumn < aStartColumn + numColumnsToClear; currentColumn++)
			{
				GO_ASSERT(currentColumn < PropagationRow::MatrixDimensions, "");

				int wrappedColumnIndex = Wrap(currentColumn, 0, myDimensions.x - 1);
				row.myCells[currentColumn].myValue = 0.0f;
			}
		}
	}

	void PropagationMatrix::clearRows(int aStartRow, int anEndRow)
	{
	}

	void PropagationMatrix::resetGrid(sf::Vector2f aCenterPoint)
	{
		for (int currentRow = 0; currentRow < myDimensions.y; currentRow++)
		{
			GO_ASSERT(currentRow < PropagationRow::MatrixDimensions, "");
			PropagationRow& row = myRows[currentRow];
			for (int currentColumn = 0; currentColumn < myDimensions.x; currentColumn++)
			{
				GO_ASSERT(currentColumn < PropagationRow::MatrixDimensions, "");
				row.myCells[currentColumn].myValue = 0.0f;
			}
		}

		myCenter = aCenterPoint;
		myCenterCell = (myDimensions - sf::Vector2i(1, 1)) / 2;
	}

	sf::Vector2i PropagationMatrix::mapPositionToUnconstrainedGridCoordinates(sf::Vector2f aPosition) const
	{
		sf::Vector2f localPosition = aPosition - myCenter;

		sf::Vector2i gridCoordinates;

		gridCoordinates.x = std::lround(localPosition.x / (myCellHalfSize * 2));
		gridCoordinates.y = std::lround(localPosition.y / (myCellHalfSize * 2));

		gridCoordinates += myCenterCell;

		return gridCoordinates;
	}

	sf::Vector2i PropagationMatrix::mapPositionToGridCoordinates(sf::Vector2f aPosition) const
	{
		sf::Vector2i gridCoordinates = mapPositionToUnconstrainedGridCoordinates(aPosition);

		gridCoordinates.x = Wrap(gridCoordinates.x, 0, myDimensions.x - 1);
		gridCoordinates.y = Wrap(gridCoordinates.y, 0, myDimensions.y - 1);

		return gridCoordinates;
	}

	sf::Vector2f PropagationMatrix::mapGridCoordinateToPosition(sf::Vector2i aGridCoordinate) const
	{		
		sf::Vector2i minRealGridCoordinate = myCenterCell - ((myDimensions - sf::Vector2i(1, 1)) / 2);
		sf::Vector2i maxRealGridCoordinate = myCenterCell + ((myDimensions - sf::Vector2i(1, 1)) / 2);

		sf::Vector2i remappedGridCoordinate;
		remappedGridCoordinate.x = Wrap(aGridCoordinate.x, minRealGridCoordinate.x, maxRealGridCoordinate.x);
		remappedGridCoordinate.y = Wrap(aGridCoordinate.y, minRealGridCoordinate.y, maxRealGridCoordinate.y);

		sf::Vector2i distanceToCenter = remappedGridCoordinate - myCenterCell;

		sf::Vector2f position;
		position = myCenter;
		position.x += distanceToCenter.x * (myCellHalfSize * 2.0f);
		position.y += distanceToCenter.y * (myCellHalfSize * 2.0f);

		return position;
	}

	void PropagationMatrix::debugDraw()
	{
		sf::RenderWindow& renderWindow = GO::GameInstance::GetInstance()->getMainWindow();
		sf::Text text;
		text.setFont(GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont());

		for (int row = 0; row < myDimensions.y; row++)
		{
			for (int column = 0; column < myDimensions.x; column++)
			{
				sf::Vector2f position = mapGridCoordinateToPosition(sf::Vector2i(column, row));

				GO_ASSERT(row < PropagationRow::MatrixDimensions, "");
				GO_ASSERT(column < PropagationRow::MatrixDimensions, "");
				std::stringstream ss;
				ss << myRows[row].myCells[column].myValue;
				text.setString(ss.str());

				text.setPosition(position);
				renderWindow.draw(text);
			}
		}
	}

	void PropagationMatrix::updateCells()
	{
		for (int row = 0; row < myDimensions.y; row++)
		{
			for (int col = 0; col < myDimensions.x; col++)
			{
				//myRows[row].myCells[col].update();
				myRows[row].myCells[col].myValue -= 0.001f;
			}
		}
	}
}