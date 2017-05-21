#include "StableHeaders.h"
#include "GO_ProfilerRenderer.h"
#include "GO_ProfilerTags.h"

// TODO(scarroll): These are here to support getting the default font for the label rendering. Remove this dependency
#include "GO_GameInstance.h"
#include "GO_HacksGlobalResources.h"

namespace GO_ProfilerRenderer
{
	const long long ourProfilerMergeTolerance = 1000;

	struct ThreadSpecificEventInfo
	{
		ThreadSpecificEventInfo()
			: myLastEventStartTime(-1)
			, myThreadIndex(std::numeric_limits<unsigned int>::max())
			, myLastThreadTag(std::numeric_limits<unsigned char>::max())
		{
		}
		long long myLastEventStartTime;
		unsigned int myThreadIndex;
		unsigned char myLastThreadTag;
	};

#pragma optimize("", off)
	void ValidateProfilerThreadEvent(const ProfilerThreadEvent& aPTE, long long aFrameEndTime, const ThreadSpecificEventInfo& aThreadSpecificInfo)
	{
		GO_ASSERT(aPTE.myThreadIndex == GetCurrentThreadIndex() || aPTE.myStartTime <= aFrameEndTime  || aThreadSpecificInfo.myLastThreadTag <= 1, "An event went beyond the 'main' we are done event");
		//if (!(aPTE.myThreadIndex == GetCurrentThreadIndex() || pte.myStartTime <= mainThreadTime || threadSpecific.myLastThreadTag <= 1))
		//{
		//	int* test = nullptr;
		//	(*test) = 5;
		//}
	}
#pragma optimize("", on)

	void RenderProfilerNode(long long aProfiledTime, const float aParentTotalTime, unsigned int aColor, const sf::Vector2f& aRegionSize, sf::Vector2f& aCurrentOffset, sf::RenderWindow& aWindow)
	{
		const float childTime = GO_APIProfiler::TicksToMilliseconds(aProfiledTime);
		const float childRectangleWidth = aRegionSize.x * (childTime / aParentTotalTime);

		sf::RectangleShape profilerOutline(sf::Vector2f(childRectangleWidth, aRegionSize.y));
		profilerOutline.setPosition(sf::Vector2f(aCurrentOffset));
		aCurrentOffset.x += childRectangleWidth;

		profilerOutline.setFillColor(sf::Color(aColor));

		aWindow.draw(profilerOutline);
	}

	void RenderProfilerLevel(ProfilerNode* aLevelParentNode, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow)
	{
		const float parentTotalTime = GO_APIProfiler::TicksToMilliseconds(aLevelParentNode->myAccumulator);
		sf::Vector2f currentOffset = aStartingPosition;

		std::vector<ProfilerNode*> reversedOrderChildren;
		// Render children nodes
		ProfilerNode* childNode = aLevelParentNode->myFirstChild;
		while (childNode)
		{
			reversedOrderChildren.push_back(childNode);
			childNode = childNode->mySibling;
		}

		while (reversedOrderChildren.size())
		{
			ProfilerNode* childNode = reversedOrderChildren.back();
			reversedOrderChildren.erase(reversedOrderChildren.end() - 1);

			RenderProfilerNode(childNode->myAccumulator, parentTotalTime, childNode->myColor, aRegionSize, currentOffset, aWindow);
			childNode = childNode->mySibling;
		}

		// Render a node that is the 'unknown' time that wasn't tracked for this level
		// Count up the time of the children
		long long totalTicks = 0;
		{
			ProfilerNode* childNode = aLevelParentNode->myFirstChild;
			while (childNode)
			{
				totalTicks += childNode->myAccumulator;
				childNode = childNode->mySibling;
			}
		}

		const long long unknownTime = aLevelParentNode->myAccumulator - totalTicks;
		const float unknwonRectangleWidth = aRegionSize.x * (unknownTime / parentTotalTime);

		RenderProfilerNode(unknownTime, parentTotalTime, 0xffffffff, aRegionSize, currentOffset, aWindow);
	}

	sf::Color locGetThreadTagColor(unsigned char aThreadTag)
	{
		switch (aThreadTag)
		{
			// NOTE: This is the 'unknown' case and we should really be pushing something on here
			// 0xRRGGBBAA
		case GO_ProfilerTags::THREAD_TAG_UNKNOWN:
			return sf::Color(0xffffffff);
		case GO_ProfilerTags::THREAD_TAG_IDLE:
			return sf::Color(0x777777ff);
		case GO_ProfilerTags::THREAD_TAG_WAITING:
			return sf::Color(0xff000099);
		case GO_ProfilerTags::THREAD_TAG_OVERHEAD:
			return sf::Color(0xff4499ff);
		case GO_ProfilerTags::THREAD_TAG_TASK_SCHEDULER:
			return sf::Color(0xffff00ff);
		case GO_ProfilerTags::THREAD_TAG_PROPAGATION:
			return sf::Color(0x00ffffff);
		case GO_ProfilerTags::THREAD_TAG_GAME_TASK:
			return sf::Color(0xff0000ff);
		case GO_ProfilerTags::THREAD_TAG_MAIN_THREAD:
			return sf::Color(0x009900ff);
		case GO_ProfilerTags::THREAD_TAG_PROFILER_RENDER:
			return sf::Color(0xaa00ffff);
		case GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK:
			return sf::Color(0x00ff00ff);
		case GO_ProfilerTags::THREAD_TAG_CALC_SYNC_POINT_TASK:
			return sf::Color(0x33ee55ff);
		case GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK:
			return sf::Color(0x0000ffff);
		case GO_ProfilerTags::THREAD_TAG_APPLY_SYNC_POINT_TASK:
			return sf::Color(0xff7700ff);
		default:
			return sf::Color(0xffffffff);
		}
	}

	sf::String locGetThreadTagLabel(unsigned char aThreadTag)
	{
		switch (aThreadTag)
		{
			// NOTE: This is the 'unknown' case and we should really be pushing something on here
			// 0xRRGGBBAA
		case GO_ProfilerTags::THREAD_TAG_UNKNOWN:
			return "Unknown";
		case GO_ProfilerTags::THREAD_TAG_IDLE:
			return "Idle";
		case GO_ProfilerTags::THREAD_TAG_WAITING:
			return "Waiting";
		case GO_ProfilerTags::THREAD_TAG_OVERHEAD:
			return "Overhead";
		case GO_ProfilerTags::THREAD_TAG_TASK_SCHEDULER:
			return "Task Scheduler";
		case GO_ProfilerTags::THREAD_TAG_PROPAGATION:
			return "Propagation";
		case GO_ProfilerTags::THREAD_TAG_GAME_TASK:
			return "Game Task";
		case GO_ProfilerTags::THREAD_TAG_MAIN_THREAD:
			return "Main Thread";
		case GO_ProfilerTags::THREAD_TAG_PROFILER_RENDER:
			return "Profiler Render:";
		case GO_ProfilerTags::THREAD_TAG_CALC_GAME_TASK:
			return "Calc Game Task";
		case GO_ProfilerTags::THREAD_TAG_APPLY_GAME_TASK:
			return "Apply Game Task";
		case GO_ProfilerTags::THREAD_TAG_APPLY_SYNC_POINT_TASK:
			return "Apply Game Sync Point";
		default:
			return "Un-named";
		}
	}

	void RenderThreadEvents(const std::vector<ProfilerThreadEvent>& someThreadEvents, const long long aFrameStartTime, const long long aFrameEndTime, const sf::Vector2f& aStartingPosition, const sf::Vector2f& aRegionSize, sf::RenderWindow& aWindow)
	{
		const long long frameDuration = aFrameEndTime - aFrameStartTime;
		GO_ASSERT(frameDuration > 0, "Previous frame must have a duration");

		static std::vector<ThreadSpecificEventInfo> threadSpecificInfos;

		// Draw a background over the known threads to make it easier to see
		sf::Vector2f rectSize;
		rectSize.x = aRegionSize.x;
		rectSize.y = 15.0f * threadSpecificInfos.size();

		sf::Vector2f rectPosition;
		rectPosition.x = aStartingPosition.x;
		rectPosition.y = aStartingPosition.y;

		sf::RectangleShape profilerOutline(rectSize);
		profilerOutline.setPosition(rectPosition);
		profilerOutline.setFillColor(sf::Color(0x000000BB));
		aWindow.draw(profilerOutline);


		// Reset all of our thread infos that we know about since we are starting a new frame
		for (size_t lookupIndex = 0; lookupIndex < threadSpecificInfos.size(); lookupIndex++)
		{
			auto& tsi = threadSpecificInfos[lookupIndex];
			tsi.myLastEventStartTime = aFrameStartTime;
			// TODO(scarroll): Validate that the tags are all set to idle perhaps?
			//tsi.myLastThreadTag = GO_ProfilerTags::THREAD_TAG_IDLE;
		}

		unsigned int numDrawn = 0;
		for (const ProfilerThreadEvent& pte : someThreadEvents)
		{
			GO_ASSERT(pte.myStartTime >= aFrameStartTime, "Received an event that started before the frame did");

			// As we go over every event, keep track of the threads that we encounter so we know where to put the markers
			size_t lookupIndex = 0;
			for (lookupIndex = 0; lookupIndex < threadSpecificInfos.size(); lookupIndex++)
			{
				if (threadSpecificInfos[lookupIndex].myThreadIndex == pte.myThreadIndex)
				{
					break;
				}
			}

			// We couldn't find a thread specific info entry for this thread so we have to create a new one
			if (lookupIndex == threadSpecificInfos.size())
			{
				threadSpecificInfos.push_back(ThreadSpecificEventInfo());

				lookupIndex = threadSpecificInfos.size() - 1;

				ThreadSpecificEventInfo& newThreadInfo = threadSpecificInfos[lookupIndex];
				newThreadInfo.myThreadIndex = pte.myThreadIndex;
				newThreadInfo.myLastEventStartTime = aFrameStartTime;
				newThreadInfo.myLastThreadTag = GO_ProfilerTags::THREAD_TAG_IDLE;
			}

			ThreadSpecificEventInfo& threadSpecific = threadSpecificInfos[lookupIndex];

			// Since we are only storing the starting time of each event, we can't draw it until we encounter
			// the next event on the time line.

			// Skip elements that are under our tolerance to avoid creating tons of small rectangles. Merge them
			// into one larger rectangle

			ValidateProfilerThreadEvent(pte, aFrameEndTime, threadSpecific);

			// TODO: The start time may be before this frame has actually started.
			//		 This would indicate an event that was carried forward from the
			//		 previous frame
			const long long eventStartTime = threadSpecific.myLastEventStartTime - aFrameStartTime;
			const long long eventEndTime = pte.myStartTime - aFrameStartTime;

			GO_ASSERT(eventStartTime >= 0, "Event started before the frame and this is currently unsupported");
			GO_ASSERT(eventEndTime >= 0, "Event ended before our frame started");
			GO_ASSERT(eventEndTime <= frameDuration, "Event ended after the frame it was contained in did");

			const long long eventDuration = eventEndTime - eventStartTime;
			GO_ASSERT(eventDuration >= 0, "The event took 'negative' time");

			// Skip over any that can be merged together
			// TODO(scarroll): We need to keep track of the 'current' end time so that we can compare to a moving end point for merging
			if (pte.myThreadTag == threadSpecific.myLastThreadTag &&
				(pte.myStartTime - threadSpecific.myLastEventStartTime) < ourProfilerMergeTolerance)
			{
				continue;
			}

			const double eventLengthPercentage = (eventDuration / (float)frameDuration);
			GO_ASSERT(eventLengthPercentage >= 0.0, "Event was calculated as taking negative percent of a frame");
			GO_ASSERT(eventLengthPercentage <= 1.0, "Event was calculated as taking more than 100% of a frame");

			const double eventStartPercentage = (eventStartTime / (float)frameDuration);
			GO_ASSERT(eventLengthPercentage >= 0.0, "Event was calculated as starting before a frame");
			GO_ASSERT(eventLengthPercentage <= 1.0, "Event was calculated as starting after a frame");

			sf::Vector2f rectSize;
			rectSize.x = aRegionSize.x * eventLengthPercentage;
			rectSize.y = 10.0f;

			sf::Vector2f rectPosition;
			rectPosition.x = aStartingPosition.x + (aRegionSize.x * eventStartPercentage);
			rectPosition.y = aStartingPosition.y + (threadSpecific.myThreadIndex * 15.0f);

			GO_ASSERT(rectPosition.x >= aStartingPosition.x, "The event will be drawn before the timeline");
			GO_ASSERT(rectSize.x >= 0.0f, "The event will be drawn with negative size");
			GO_ASSERT(rectPosition.x <= aStartingPosition.x + aRegionSize.x, "Bar goes beyond the bounds given to us");
			//GO_ASSERT((rectPosition.x + rectSize.x) <= (aStartingPosition.x + aRegionSize.x), "Bar extends beyond the bounds of the profiler display");

			sf::RectangleShape profilerOutline(rectSize);
			profilerOutline.setPosition(rectPosition);

			sf::Color eventColor = locGetThreadTagColor(threadSpecific.myLastThreadTag);
			profilerOutline.setFillColor(eventColor);

			aWindow.draw(profilerOutline);
			numDrawn++;

			threadSpecific.myLastEventStartTime = pte.myStartTime;
			threadSpecific.myLastThreadTag = pte.myThreadTag;
		}

		// TODO: Render the last events for each of the threads
		for (size_t lookupIndex = 0; lookupIndex < threadSpecificInfos.size(); lookupIndex++)
		{
			ThreadSpecificEventInfo& threadSpecific = threadSpecificInfos[lookupIndex];
			if (threadSpecific.myLastThreadTag <= 1)
			{
				continue;
			}

			//	// Since we are only storing the starting time of each event, we can't draw it until we encounter
			//	// the next event on the time line.

			// TODO: The start time may be before this frame has actually started.
			//		 This would indicate an event that was carried forward from the
			//		 previous frame
			const long long eventStartTime = threadSpecific.myLastEventStartTime - aFrameStartTime;
			const long long eventEndTime = aFrameEndTime - aFrameStartTime;

			GO_ASSERT(eventStartTime >= 0, "Event started before the frame and this is currently unsupported");
			GO_ASSERT(eventEndTime >= 0, "Event ended before our frame started");
			GO_ASSERT(eventEndTime <= frameDuration, "Event ended after the frame it was contained in did");

			const long long eventDuration = eventEndTime - eventStartTime;
			GO_ASSERT(eventDuration >= 0, "The event took 'negative' time");

			const double eventLengthPercentage = (eventDuration / (float)frameDuration);
			GO_ASSERT(eventLengthPercentage >= 0.0, "Event was calculated as taking negative percent of a frame");
			GO_ASSERT(eventLengthPercentage <= 1.0, "Event was calculated as taking more than 100% of a frame");

			const double eventStartPercentage = (eventStartTime / (float)frameDuration);
			GO_ASSERT(eventLengthPercentage >= 0.0, "Event was calculated as starting before a frame");
			GO_ASSERT(eventLengthPercentage <= 1.0, "Event was calculated as starting after a frame");

			sf::Vector2f rectSize;
			rectSize.x = aRegionSize.x * eventLengthPercentage;
			rectSize.y = 10.0f;

			sf::Vector2f rectPosition;
			rectPosition.x = aStartingPosition.x + (aRegionSize.x * eventStartPercentage);
			rectPosition.y = aStartingPosition.y + (threadSpecific.myThreadIndex * 15.0f);

			GO_ASSERT(rectPosition.x >= aStartingPosition.x, "The event will be drawn before the timeline");
			GO_ASSERT(rectSize.x >= 0.0f, "The event will be drawn with negative size");
			GO_ASSERT(rectPosition.x <= (aStartingPosition.x + aRegionSize.x), "Bar goes beyond the bounds given to us");
			//GO_ASSERT((rectPosition.x + rectSize.x) <= (aStartingPosition.x + aRegionSize.x), "Bar extends beyond the bounds of the profiler display");

			sf::RectangleShape profilerOutline(rectSize);
			profilerOutline.setPosition(rectPosition);

			sf::Color eventColor = locGetThreadTagColor(threadSpecific.myLastThreadTag);
			profilerOutline.setFillColor(eventColor);

			aWindow.draw(profilerOutline);
			numDrawn++;
		}
	}

	void GO_ProfilerRenderer::RenderThreadEventTitles(const std::vector<ProfilerThreadEvent>& someThreadEvents, sf::Vector2f aStartingPosition, sf::RenderWindow& aWindow)
	{
		sf::Font& font = GO::GameInstance::GetInstance()->getHacksGlobalResources().getDefaultFont();

		for(unsigned char currentTag = 0; currentTag < GO_ProfilerTags::THREAD_TAG_COUNT; currentTag++)
		{
			sf::Text text;
			text.setFont(font);
			text.setCharacterSize(16);
			text.setColor(locGetThreadTagColor(currentTag));
			text.setPosition(aStartingPosition);

			// TODO(scarroll): Use the proper font height to give nice line spacing in profiler labels
			aStartingPosition.y += 16.0f;

			//char buffer[16];
			//_i64toa_s(currentTag, buffer, 16, 10);

			text.setString(locGetThreadTagLabel(currentTag));
			//text.setString(buffer);

			aWindow.draw(text);
		}
	}
}