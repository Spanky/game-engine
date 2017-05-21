#pragma once

namespace sf
{
	class RenderWindow;
}

class GO_APIProfiler;

namespace GO
{
	class World;
	class ThreadPool;
	class GameInstance;

	struct SystemUpdateParams
	{
		SystemUpdateParams(GO::World& aWorld, GO_APIProfiler& aProfiler, GO::ThreadPool& aThreadPool, sf::RenderWindow& aWindow, GO::GameInstance& aGameInstance, sf::Time aDeltaTime)
			: myWorld(aWorld)
			, myProfiler(aProfiler)
			, myThreadPool(aThreadPool)
			, myRenderWindow(aWindow)
			, myGameInstance(aGameInstance)
			, myDeltaTime(aDeltaTime)
		{
		};

		GO::World& myWorld;
		GO_APIProfiler& myProfiler;
		GO::ThreadPool& myThreadPool;

		// TODO(scarroll): This should be phased out in case split screen is ever a requirement
		sf::RenderWindow& myRenderWindow;
		GO::GameInstance& myGameInstance;

		sf::Time myDeltaTime;
	};


	template<typename T>
	struct GameUpdateSystemTypeTraits
	{
	};

	template<typename T>
	struct ProfilerTypeTraits
	{
	};

	struct GameUpdateSystemNoDependencies
	{
		void GatherTaskDependencies(std::vector<unsigned int>& someOutDependencies)
		{
		}
	};

	template<unsigned int TaskDependency>
	struct GameUpdateSystemOneDependencies
	{
		void GatherTaskDependencies(std::vector<unsigned int>& someOutDependencies)
		{
			someOutDependencies.push_back(TaskDependency);
		}
	};


	class GameUpdateSystem
	{
	public:
		virtual void updateSystem(SystemUpdateParams& someUpdateParams) = 0;
	};
}