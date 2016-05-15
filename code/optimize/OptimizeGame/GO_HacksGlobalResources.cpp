#include "StableHeaders.h"
#include "GO_HacksGlobalResources.h"

#include "GO_GameInstance.h"
#include "GO_Gameworks.h"

namespace
{
	std::ostringstream& getErrorStream()
	{
		return GO::GameInstance::GetInstance()->getGameworks().mySFMLOutput;
	}

	void printLastErrorToDebug()
	{
		std::string error = getErrorStream().str();
		OutputDebugString(error.c_str());
	}


	static bool loadFontFromFile(const char* aFileName, sf::Font& aFont)
	{
		std::string fullPath = GO::GameInstance::GetInstance()->getGameworks().myDataDirectory + std::string(aFileName);

		bool loadResult = aFont.loadFromFile(fullPath);
		if (!loadResult)
		{
			printLastErrorToDebug();
			assert(false);
		}

		return loadResult;
	}

	static bool loadTextureFromFile(const char* aFileName, sf::Texture& aTexture)
	{
		std::string fullPath = GO::GameInstance::GetInstance()->getGameworks().myDataDirectory + std::string(aFileName);

		bool loadResult = aTexture.loadFromFile(fullPath);
		if (!loadResult)
		{
			printLastErrorToDebug();
			assert(false);
		}

		return loadResult;
	}
}

namespace GO
{
	HacksGlobalResources::HacksGlobalResources()
	{
		LoadResources();
	}

	void HacksGlobalResources::LoadResources()
	{
		loadFontFromFile("fonts\\sansation.ttf", myFont);
		loadTextureFromFile("sprites\\characters\\link.png", myLinkSpriteTexture);
	}
}