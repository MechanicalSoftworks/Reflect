#include "Core/Allocator.h"
#include "ReflectStructs.h"

#include <memory>

namespace Reflect
{
	IReflect* Allocator::CreateInternal(const Class* static_class, IReflect* outer)
	{
		IReflect* o = static_class->Allocate();
		if (!o)
		{
			throw std::bad_alloc();
		}

		const Initialiser init(static_class, outer);
		static_class->Constructor(o, init);
		return o;
	}

	void Allocator::DestroyInternal(IReflect* o)
	{
		if (o)
		{
			const auto* static_class = o->GetClass();
			static_class->Destructor(o);
			static_class->Free(o);
		}
	}

	const Class* Allocator::Lookup(const std::string_view& name)
	{
		const auto* static_class = Class::Lookup(name);
		if (!static_class)
		{
			throw std::runtime_error(std::string("Unknown class '") + std::string(name) + "'");
		}

		return static_class;
	}
}