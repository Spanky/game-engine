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


BENCHMARK_MAIN()