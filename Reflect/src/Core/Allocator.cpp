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
	IReflect* Allocator::CreateInternal(const Class* static_class)
	{
		IReflect* o = (IReflect *)aligned_alloc(static_class->GetRawSize(), static_class->GetAlignment());
		if (!o)
		{
			throw std::bad_alloc();
		}

		static_class->Constructor(o);
		o->Initialise(static_class);
		return o;
	}

	void Allocator::DestroyInternal(IReflect* o)
	{
		if (o)
		{
			const auto* static_class = o->GetClass();
			static_class->Destructor(o);
			aligned_free(o);
		}
	}
}