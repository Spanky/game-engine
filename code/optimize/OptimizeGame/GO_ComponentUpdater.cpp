#include "StableHeaders.h"
#include "GO_ComponentUpdater.h"

#include "GO_EntityComponent.h"
#include "GO_SpriteComponent.h"
#include "GO_GameInstance.h"
#include "GO_HacksGlobalResources.h"

namespace GO
{
	static const int Int64TextWidth = 20;

	void drawFpsCounter(sf::RenderWindow& aWindow, const sf::Font& aFont, sf::Time aDeltaTime)
	{
		sf::Text text;
		text.setFont(aFont);
		text.setColor(sf::Color::Red);
		text.setPosition(0, 50.0f);

		char buffer[Int64TextWidth];
		_i64toa_s(aDeltaTime.asMicroseconds(), buffer, Int64TextWidth, 10);
		//_gcvt_s(buffer, Int64TextWidth, elapsedTime, 20);

		text.setString(buffer);

		aWindow.draw(text);
	}



	void ComponentUpdater::registerComponent(EntityComponent* aComponent)
	{
		myComponents.push_back(aComponent);
	}

	void ComponentUpdater::unregisterComponent(EntityComponent* aComponent)
	{
		const auto searchIter = std::find(myComponents.begin(), myComponents.end(), aComponent);
		GO_ASSERT(searchIter != myComponents.end(), "Component was not registered with the updater system");

		myComponents.erase(searchIter);
	}

	void ComponentUpdater::updateComponents()
	{
		//if(mySprites.size() != myComponents.size())
		//{
		//	sf::Texture& linkTexture = GameInstance::GetInstance()->getHacksGlobalResources().getLinkSpriteTexture();

		//	mySprites.reserve(myComponents.size());
		//	for (size_t i = 0; i < myComponents.size(); i++)
		//	{
		//		sf::Sprite currentSprite;

		//		currentSprite.setTexture(linkTexture);
		//		currentSprite.setTextureRect(sf::IntRect(0, 7, 26, 26));
		//		currentSprite.setScale(5.0f, 5.0f);

		//		mySprites.push_back(currentSprite);
		//	}
		//}

		sf::Clock updateTimer;

		for (EntityComponent* currentComponent : myComponents)
		{
			currentComponent->updateComponent();
		}

		//sf::RenderWindow& renderWindow = GameInstance::GetInstance()->getMainWindow();
		//for(sf::Sprite& currentSprite : mySprites)
		//{
		//	renderWindow.draw(currentSprite);
		//}

		sf::Time deltaTime = updateTimer.restart();
		double elapsedTime = static_cast<double>(deltaTime.asMicroseconds()) / 1000.0;

		drawFpsCounter(GO::GameInstance::GetInstance()->getMainWindow(), GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont(), deltaTime);
	}
}