#include "StableHeaders.h"

#include "GO_Entity.h"
#include "GO_EntityComponent.h"

#include "GO_GameInstance.h"
#include "GO_TaskScheduler.h"

static std::atomic_size_t ourNextEntityId = 0;

namespace GO
{
	Entity::Entity()
	{
		myId = std::atomic_fetch_add(&ourNextEntityId, 1);
	}

	Entity::~Entity()
	{
		GO_ASSERT(myComponents.size() == 0, "Components must be destroyed before an entity is destroyed");
	}

	void Entity::destroyAllComponents()
	{
		for(EntityComponent* currentComponent : myComponents)
		{
			GameInstance::GetInstance()->getTaskScheduler().unregisterComponent(currentComponent);
			delete currentComponent;
		}
		myComponents.clear();
	}

	void Entity::setPosition(const sf::Vector2i& aPosition)
	{
		myPosition = aPosition;
	}

	void Entity::setScale(const sf::Vector2i& aScale)
	{
		myScale = aScale;
	}

	void Entity::setRotation(const sf::Vector2i& aRotation)
	{
		myRotation = aRotation;
	}
}