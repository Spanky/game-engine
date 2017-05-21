#pragma once

namespace GO
{
	class EntityComponent;

	
	class Entity
	{
	public:
		Entity();
		~Entity();

		const size_t getId() const { return myId; }

		template<typename ComponentType>
		ComponentType* createComponent();

		template<typename ComponentType>
		ComponentType* getComponent();

		template<typename ComponentType>
		const ComponentType* getComponent() const;

		void destroyAllComponents();

		const sf::Vector2i& getPosition() const { return myPosition; }
		void setPosition(const sf::Vector2i& aPosition);

		const sf::Vector2i& getScale() const { return myScale; }
		void setScale(const sf::Vector2i& aScale);

		const sf::Vector2i& getRotation() const { return myRotation; }
		void setRotation(const sf::Vector2i& aRotation);

	private:
		size_t myId;

		std::vector<EntityComponent*> myComponents;

		sf::Vector2i myPosition;
		sf::Vector2i myScale;
		sf::Vector2i myRotation;
	};



	template<typename ComponentType>
	ComponentType* Entity::createComponent()
	{
		// TODO: This should be in some sort of system that will be responsible for creating, initializing, and registering the component with updaters
		ComponentType* component = new ComponentType(*this);
		myComponents.push_back(component);

		GameInstance::GetInstance()->getTaskScheduler().registerComponent(component);

		return component;
	}

	template<typename ComponentType>
	ComponentType* Entity::getComponent()
	{
		for(EntityComponent* currentComponent : myComponents)
		{
			if(dynamic_cast<ComponentType*>(currentComponent))
			{
				return static_cast<ComponentType*>(currentComponent);
			}
		}

		return nullptr;
	}

	template<typename ComponentType>
	const ComponentType* Entity::getComponent() const
	{
		for (EntityComponent* currentComponent : myComponents)
		{
			if (dynamic_cast<ComponentType*>(currentComponent))
			{
				return static_cast<ComponentType*>(currentComponent);
			}
		}

		return nullptr;
	}
}