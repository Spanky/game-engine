#pragma once

#include "GO_Profiler.h"

namespace GO_ProfilerRenderer
{
	void RenderThreadEvents(const std::vector<ProfilerThreadEvent>& someThreadEvents, const long long aFrameStartTime, const long long aFrameEndTime, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow);
	void RenderThreadEventTitles(const std::vector<ProfilerThreadEvent>& someThreadEvents, sf::Vector2f aStartingPosition, sf::RenderWindow& aWindow);
	void RenderProfiler(GO_APIProfiler& aProfiler, const GO_APIProfiler::FrameCallGraphRoots& someRootNodes, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow);
}
