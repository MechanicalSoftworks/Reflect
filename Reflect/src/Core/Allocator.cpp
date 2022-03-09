#include "Core/Allocator.h"
#include "ReflectStructs.h"

#include <memory>

#ifdef __GNUC__
void* local_aligned_alloc(std::size_t size, std::size_t alignment) { return std::aligned_alloc(size, alignment); }
void  local_aligned_free(void* p) noexcept { std::free(p); }
#else
void* local_aligned_alloc(std::size_t size, std::size_t alignment) { return _aligned_malloc(size, alignment); }
void  local_aligned_free(void* p) noexcept { _aligned_free(p); }
#endif

namespace Reflect
{
	IReflect* Allocator::CreateInternal(const Class* static_class, IReflect* outer)
	{
		IReflect* o = (IReflect *)local_aligned_alloc(static_class->GetRawSize(), static_class->GetAlignment());
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
			local_aligned_free(o);
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