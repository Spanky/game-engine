#pragma once

#include "GO_ComponentAccessFlags.h"
#include "GO_ComponentTypes.h"

namespace GO
{
	// A tuple of components for read-access
	template<typename...ComponentTypes>
	struct ReadComponentList {};

	// A tuple of components for write-access
	template<typename...ComponentTypes>
	struct WriteComponentList {};


	class ComponentAccessControl
	{
	private:
		// Allow 0 parameters
		template<typename...ComponentTypes>
		struct WriteAccessGranter;

		// Traversal of types
		template<typename ComponentType, typename...AdditionalComponentTypes>
		struct WriteAccessGranter<ComponentType, AdditionalComponentTypes...>
		{
			WriteAccessGranter(std::shared_ptr<ComponentAccessFlags> anAccessFlags)
			{
				anAccessFlags->grantWriteAccess(ComponentType::getComponentTypeStatic());
				WriteAccessGranter<AdditionalComponentTypes...> granter(anAccessFlags);
			}
		};

		// Recursion termination
		template<>
		struct WriteAccessGranter<>
		{
			WriteAccessGranter(std::shared_ptr<ComponentAccessFlags> anAccessFlags)
			{
			}
		};



		// Allow 0 parameters
		template<typename...ComponentTypes>
		struct ReadAccessGranter;

		// Traversal of types
		template<typename ComponentType, typename...AdditionalComponentTypes>
		struct ReadAccessGranter<ComponentType, AdditionalComponentTypes...>
		{
			ReadAccessGranter(std::shared_ptr<ComponentAccessFlags> anAccessFlags)
			{
				anAccessFlags->grantReadAccess(ComponentType::getComponentTypeStatic());
				ReadAccessGranter<AdditionalComponentTypes...> granter(anAccessFlags);
			}
		};

		// Recursion termination
		template<>
		struct ReadAccessGranter<>
		{
			ReadAccessGranter(std::shared_ptr<ComponentAccessFlags> anAccessFlags)
			{
			}
		};


	public:
		template<typename...ReadComponentTypes, typename...WriteComponentTypes>
		static ComponentAccessFlagsType requestAccess(ReadComponentList<ReadComponentTypes...> aReadComponentList, WriteComponentList<WriteComponentTypes...> aWriteComponentList)
		{
			std::shared_ptr<ComponentAccessFlags> accessFlags = std::make_shared<ComponentAccessFlags>(ComponentAccessFlagsToken());

			ReadAccessGranter<ReadComponentTypes...> readGranter(accessFlags);
			WriteAccessGranter<WriteComponentTypes...> writeGranter(accessFlags);

			storeAccessFlags(accessFlags);

			return accessFlags;
		}

		static void releaseAccess(ComponentAccessFlagsType someAccessFlags)
		{
			std::lock_guard<std::mutex> lock(ourStorageMutex);
			auto iter = std::find(ourOutstandingFlags.begin(), ourOutstandingFlags.end(), someAccessFlags);
			GO_ASSERT(iter != ourOutstandingFlags.end(), "Could not find flags");
			ourOutstandingFlags.erase(iter);

			someAccessFlags->resetAccess();
		}

	private:
		static void storeAccessFlags(ComponentAccessFlagsType someAccessFlags)
		{
			std::lock_guard<std::mutex> lock(ourStorageMutex);

			validateNonConflictingAccessFlags(someAccessFlags);

			ourOutstandingFlags.push_back(someAccessFlags);
		}

		static void validateNonConflictingAccessFlags(ComponentAccessFlagsType someAccessFlags)
		{
			const size_t numOutstandingFlags = ourOutstandingFlags.size();
			for (size_t i = 0; i < numOutstandingFlags; i++)
			{
				for (int j = 0; j < int(ComponentType::NumComponents); j++)
				{
					const bool requestingRead = someAccessFlags->hasReadAccess(ComponentType(j));
					const bool requestingWrite = someAccessFlags->hasWriteAccess(ComponentType(j));

					const bool alreadyRead = ourOutstandingFlags[i]->hasReadAccess(ComponentType(j));
					const bool alreadyWrite = ourOutstandingFlags[i]->hasWriteAccess(ComponentType(j));

					// We cannot grant write access to a component that someone else has already been granted read or write access to
					GO_ASSERT(!(requestingWrite && (alreadyRead || alreadyWrite)), "Cannot grant write-access to a component already marked as either write-access or read-access");

					// We cannot read a component that someone else has already been granted write access
					GO_ASSERT(!(requestingRead && alreadyWrite), "Cannot grant read-access to a component already marked as write-access");
				}
			}
		}

		static std::mutex ourStorageMutex;
		static std::vector<std::shared_ptr<ComponentAccessFlags>> ourOutstandingFlags;
	};
}