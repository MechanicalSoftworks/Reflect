#pragma once

#include "Core/Core.h"

#include <string>
#include <memory>
#include <stdexcept>

namespace Reflect
{
	struct IReflect;
	class Class;

	class REFLECT_DLL Allocator
	{
		// Raw pointer manipulation.
		static IReflect*	CreateInternal(const Class* static_class, IReflect* outer);
		static void			DestroyInternal(IReflect* o);
		static const Class*	Lookup(const std::string_view& name);

	public:
		// Defined here because it's easiest. If Ref is defined above Allocator, it can't access Allocator::DestroyInternal.
		// If Ref is defined below Allocator, Allocator can't use it.
		// These are just redefined in the Reflect namespace at the bottom of this file.
		//
		// Derived from std::unique_ptr because it's when specifying a custom deleter - 
		// the std::unique_ptr default destructor is disabled! Doesn't work with the FieldImpl::read templates!
		template<typename T>
		class Ref : public std::unique_ptr<T, decltype(&Allocator::DestroyInternal)>
		{
			typedef std::unique_ptr<T, decltype(&Allocator::DestroyInternal)> SuperClass;
		public:
			Ref() : SuperClass(nullptr, &Allocator::DestroyInternal) {}
			Ref(T *t) : SuperClass(t, &Allocator::DestroyInternal) {}
			using SuperClass::SuperClass;

			template<typename C>
			Ref<C> Cast() { return Ref<C>(static_cast<C*>(this->release())); }
		};
		template<typename T> using WeakRef = std::weak_ptr<T>;

		template<typename T> static Ref<T>	Create(IReflect* outer);
		template<typename T> static Ref<T>	Create(const std::string_view& name, IReflect* outer);
		template<typename T> static Ref<T>	Create(const Class* static_class, IReflect* outer);
		template<typename T> static void	Dispose(Ref<T> &ref);
	};

	template<typename T>
	inline Allocator::Ref<T> Allocator::Create(IReflect* outer)
	{
		return Ref<T>((T*)CreateInternal(&T::StaticClass, outer));
	}

	template<typename T> 
	inline Allocator::Ref<T> Allocator::Create(const std::string_view& name, IReflect* outer)
	{
		return Create<T>(Lookup(name), outer);
	}

	template<typename T>
	inline Allocator::Ref<T> Allocator::Create(const Class* static_class, IReflect* outer)
	{
		return Ref<T>((T*)CreateInternal(static_class, outer));
	}

	template<typename T>
	inline void Allocator::Dispose(Ref<T>& ref)
	{
		ref = nullptr;
	}

	// Redefined for easier access.
	template<typename T> using Ref = Allocator::Ref<T>;
	template<typename T> using WeakRef = Allocator::WeakRef<T>;
}