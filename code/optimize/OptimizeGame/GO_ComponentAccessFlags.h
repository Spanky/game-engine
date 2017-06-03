#pragma once

#include "GO_ComponentTypes.h"

namespace GO
{
	struct ComponentAccessFlagsToken
	{
	private:
		ComponentAccessFlagsToken() {};
		friend class ComponentAccessControl;
	};


	class ComponentAccessFlags
	{
	public:
		friend class ComponentAccessControl;

		// TODO(scarroll): Make this private with a token class that only the access control system can create
		//					to prevent outside sources creating these
		ComponentAccessFlags(ComponentAccessFlagsToken&)
		{
			resetAccess();
		}

		bool hasWriteAccess(ComponentType aComponentType) const
		{
			return myWriteAccessFlags[AccessArray::size_type(aComponentType)];
		}

		bool hasReadAccess(ComponentType aComponentType) const
		{
			return myReadAccessFlags[AccessArray::size_type(aComponentType)];
		}

	private:
		void resetAccess()
		{
			myWriteAccessFlags.fill(false);
			myReadAccessFlags.fill(false);
		}

		void grantWriteAccess(ComponentType aComponentType)
		{
			myWriteAccessFlags[AccessArray::size_type(aComponentType)] = true;
		}

		void grantReadAccess(ComponentType aComponentType)
		{
			myReadAccessFlags[AccessArray::size_type(aComponentType)] = true;
		}

		ComponentAccessFlags(const ComponentAccessFlags& rhs) = delete;
		ComponentAccessFlags& operator=(const ComponentAccessFlags& rhs) = delete;

	private:
		using AccessArray = std::array<bool, size_t(ComponentType::NumComponents)>;
		AccessArray myWriteAccessFlags;
		AccessArray myReadAccessFlags;
	};

	using ComponentAccessFlagsType = std::shared_ptr<ComponentAccessFlags>;
}