#include "StableHeaders.h"
#include "GO_GameInstance.h"

#include "GO_HacksGlobalResources.h"

namespace GO
{
	static GameInstance* ourInstance;

	GameInstance* GameInstance::GetInstance()
	{
		return ourInstance;
	}

	GameInstance::GameInstance(sf::RenderWindow& aWindow, Gameworks& aGameworks)
		: myWindow(aWindow)
		, myHacksGlobalResources(nullptr)
		, myGameworks(aGameworks)
	{
		GO_ASSERT(ourInstance == nullptr, "Cannot create multiple game instances");
		ourInstance = this;

		myHacksGlobalResources = new HacksGlobalResources();
	}

	GameInstance::~GameInstance()
	{
		GO_ASSERT(ourInstance == this, "GameInstance was not the global game instance");

		delete myHacksGlobalResources;
		myHacksGlobalResources = nullptr;

		ourInstance = nullptr;
	}
}