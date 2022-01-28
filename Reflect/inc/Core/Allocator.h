#pragma once

#include "Core/Core.h"

#include <string>

namespace Reflect
{
	struct IReflect;
	class Class;

	class REFLECT_DLL Allocator
	{
	public:
		static IReflect*	Create(const std::string_view& name);
		static IReflect*	Create(const Class* static_class);
		static void			Destroy(IReflect* o);
	};
}