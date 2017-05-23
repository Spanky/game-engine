#pragma once

namespace sf
{
	class RenderWindow;
}


namespace GO
{
	class HacksGlobalResources;
	struct Gameworks;

	class GameInstance
	{
	public:
		static GameInstance* GetInstance();

		GameInstance(sf::RenderWindow& aWindow, Gameworks& aGameworks);
		~GameInstance();

		// Prevent copying
		GameInstance(const GameInstance&) = delete;
		GameInstance& operator=(const GameInstance&) = delete;


		sf::RenderWindow& getMainWindow() { return myWindow; }
		Gameworks& getGameworks() { return myGameworks; }

		HacksGlobalResources& getHacksGlobalResources() { return *myHacksGlobalResources; }

	private:
		sf::RenderWindow& myWindow;

		HacksGlobalResources* myHacksGlobalResources;

		Gameworks& myGameworks;
	};
}