#pragma once

namespace GO
{
	class Entity;

	// TODO: Find a better way to break this up. Every game state change we add is going to cause bigger
	//		 and bigger compiles as this file is used in more and more systems.
	class GameStateChange
	{
	public:
		struct MovementMessage
		{
			GO::Entity* myEntityToMove;
			sf::Vector2i myTileToMoveTo;
		};

		static std::vector<MovementMessage> ourMovementMessages;

		static void QueueMovement(GO::Entity* anEntityToMove, const sf::Vector2i& aTileIndexToMoveTo)
		{
			MovementMessage msg;
			msg.myEntityToMove = anEntityToMove;
			msg.myTileToMoveTo = aTileIndexToMoveTo;
			ourMovementMessages.push_back(msg);
		}
	};
}