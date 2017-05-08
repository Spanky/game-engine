#include "benchmark/benchmark_api.h"

// ----------------------------------------------------------------------------
// SFML Library
// ----------------------------------------------------------------------------
#include <SFML/Graphics.hpp>

#include "GO_HealthComponent.h"
#include "GO_Entity.h"
#include "GO_GameInstance.h"

static void BM_EntityCreation(benchmark::State& aState)
{
	while(aState.KeepRunning())
	{
		GO::Entity entity;
	}
}
BENCHMARK(BM_EntityCreation);



static void BM_ComponentCreation(benchmark::State& aState)
{
	while (aState.KeepRunning())
	{
		GO::Entity entity;
		entity.createComponent<GO::HealthComponent>();
		entity.destroyAllComponents();
	}
}
BENCHMARK(BM_ComponentCreation);


static void BM_ComponentUpdating(benchmark::State& aState)
{
	std::vector<GO::Entity> entities(aState.range(0));
	std::for_each(entities.begin(), entities.end(), [](GO::Entity& anEntity) { anEntity.createComponent<GO::HealthComponent>(); });

	while(aState.KeepRunning())
	{
		std::for_each(entities.begin(), entities.end(), [](GO::Entity& anEntitiy) { anEntitiy.update(); });
	}

	std::for_each(entities.begin(), entities.end(), [](GO::Entity& anEntity) {anEntity.destroyAllComponents(); });
}
BENCHMARK(BM_ComponentUpdating)->Range(8, 8 << 10)->Threads(2);
BENCHMARK(BM_ComponentUpdating)->Range(8, 8 << 10)->Threads(4);

static void BM_LinearComponents(benchmark::State& aState)
{
	GO::Entity entity;
	std::vector<GO::HealthComponent> components(aState.range(0), GO::HealthComponent(entity));

	while(aState.KeepRunning())
	{
		std::for_each(components.begin(), components.end(), [](GO::HealthComponent& aComponent) { aComponent.updateComponent(); });
	}
}

BENCHMARK(BM_LinearComponents)->Range(8, 8 << 10)->Threads(2);
BENCHMARK(BM_LinearComponents)->Range(8, 8 << 10)->Threads(4);

BENCHMARK_MAIN()