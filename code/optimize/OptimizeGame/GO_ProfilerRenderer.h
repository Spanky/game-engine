#pragma once

#include "GO_Profiler.h"

namespace GO_ProfilerRenderer
{
	void RenderThreadEvents(const std::vector<ProfilerThreadEvent>& someThreadEvents, const long long aFrameStartTime, const float aFrameDuration, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow);
	void RenderProfilerLevel(ProfilerNode* aLevelParentNode, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow);
	void RenderProfilerNode(long long aProfiledTime, const float aParentTotalTime, unsigned int aColor, const sf::Vector2f& aRegionSize, sf::Vector2f& aCurrentOffset, sf::RenderWindow& aWindow);
}
