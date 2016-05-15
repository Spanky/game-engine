#pragma once

namespace GO
{
	class HacksGlobalResources
	{
	public:
		HacksGlobalResources();

		// Disable copying
		HacksGlobalResources(const HacksGlobalResources&) = delete;
		HacksGlobalResources& operator=(const HacksGlobalResources&) = delete;

		sf::Font& getDefaultFont() { return myFont; }
		sf::Texture& getLinkSpriteTexture() { return myLinkSpriteTexture; }

	private:
		void LoadResources();

	private:
		sf::Font myFont;
		sf::Texture myLinkSpriteTexture;
	};
}