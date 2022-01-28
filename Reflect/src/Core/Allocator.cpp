#include "Core/Allocator.h"
#include "ReflectStructs.h"

#include <memory>

#ifdef __GNUC__
void* aligned_alloc(std::size_t size, std::size_t alignment) { return std::aligned_alloc(size, alignment); }
void  aligned_free(void* p) noexcept { std::free(p); }
#else
void* aligned_alloc(std::size_t size, std::size_t alignment) { return _aligned_malloc(size, alignment); }
void  aligned_free(void* p) noexcept { _aligned_free(p); }
#endif

namespace Reflect
{
	IReflect* Allocator::Create(const std::string_view& name)
	{
		const auto* static_class = Class::Lookup(name);
		if (!static_class)
		{
			return nullptr;
		}

		return Create(static_class);
	}

	IReflect* Allocator::Create(const Class* static_class)
	{
		IReflect* o = (IReflect *)aligned_alloc(static_class->GetRawSize(), static_class->GetAlignment());
		if (!o)
		{
			return nullptr;
		}

		static_class->Constructor(o);
		o->Initialise(static_class);
		return o;
	}

	void Allocator::Destroy(IReflect* o)
	{
		if (o)
		{
			const auto* static_class = o->GetClass();
			static_class->Destructor(o);
			aligned_free(o);
		}
	}
}