#pragma once

#include "Core/Core.h"
#include "Core/Util.h"

namespace Reflect
{
	struct IReflect;

	template<typename F, typename TAttributes>
	struct StaticField
	{
		using Type = F;
		using AttributesType = TAttributes;

		constexpr StaticField(const char* name, std::size_t offset, TAttributes attributes)
			: Name(name), Offset(offset), Attributes(attributes)
		{}

		const char* const	Name;
		const std::size_t	Offset;
		const TAttributes	Attributes;

		template<typename T>
		auto& Get(T& t) const
		{
			return *reinterpret_cast<Type*>(reinterpret_cast<char*>(&t) + Offset);
		}

		template<Util::StringLiteral flag, Util::StringLiteral... TFlags>
		constexpr auto HasAnyFlag() const -> bool
		{
			if (Util::any_of(Attributes, [&](const auto& x) { return std::string_view(x) == std::string_view(flag.value); }))
			{
				return true;
			}

			if constexpr (sizeof...(TFlags))
			{
				return HasAnyFlag<TFlags...>();
			}
			else
			{
				return false;
			}
		}
	};

	template<typename F>
	constexpr auto make_static_field(const char* name, std::size_t offset, auto attributes)
	{
		return StaticField<F, decltype(attributes)>(name, offset, attributes);
	}

	namespace detail
	{
		template<typename T, int I, Reflect::Util::StringLiteral... flags>
		struct FilterProperties;

		template<typename T, Reflect::Util::StringLiteral... flags>
		struct FilterProperties<T, 0, flags...>
		{
			static constexpr auto call()
			{
				return std::make_tuple();
			}
		};

		template<typename T, int I, Reflect::Util::StringLiteral... flags>
		struct FilterProperties
		{
			static constexpr auto call()
			{
				constexpr auto properties = Reflect::ReflectStatic<T>::Properties;
				if constexpr (std::get<I - 1>(Reflect::ReflectStatic<T>::Properties).template HasAnyFlag<flags...>())
				{
					return std::tuple_cat(
						std::make_tuple(std::get<I - 1>(properties)),
						FilterProperties<T, I - 1, flags...>::call()
					);
				}
				else
				{
					return FilterProperties<T, I - 1, flags...>::call();
				}
			}
		};
	}

	template<typename T, Reflect::Util::StringLiteral... flags>
	constexpr auto FilterProperties()
	{
		constexpr auto properties = Reflect::ReflectStatic<T>::Properties;

		constexpr auto property_count = std::tuple_size<decltype(properties)>::value;
		return detail::FilterProperties<T, property_count, flags...>::call();
	}

	//
	// This version has no filter.
	//
	void ForEachProperty(auto&& obj, IReflect& t) {}
	void ForEachProperty(auto&& obj, auto& t)
	{
		using T = std::remove_reference<decltype(t)>::type;
		
		// https://stackoverflow.com/a/54053084
		std::apply(
			[&t, &obj](auto&&... args) {
				((obj(args.Name, args.Get(t)), ...));
			},
			ReflectStatic<T>::Properties
		);
		ForEachProperty(std::move(obj), static_cast<T::SuperClass&>(t));
	}

	//
	// Filter properties based on attributes.
	//
	template<Reflect::Util::StringLiteral... flags> requires Util::StringLiteralList<flags...>
	void ForEachProperty(auto&& obj, Reflect::IReflect& t) {}
	template<Reflect::Util::StringLiteral... flags> requires Util::StringLiteralList<flags...>
	void ForEachProperty(auto&& obj, auto& t)
	{
		using T = std::remove_reference<decltype(t)>::type;

		constexpr auto properties = FilterProperties<T, flags...>();
		
		// https://stackoverflow.com/a/54053084
		std::apply(
			[&t, &obj](auto&&... args) {
				((obj(args.Name, args.Get(t)), ...));
			},
			properties
		);

		ForEachProperty<flags...>(std::move(obj), static_cast<T::SuperClass&>(t));
	}

	template<typename T, typename TOther>
	constexpr inline bool IsOrDescendantOf()
	{
		if constexpr (std::is_same<T, TOther>::value)
		{
			return true;
		}
		// <T = IReflect, TOther = IReflect> is captured above.
		// This case prevents us from trying to lookup SuperClass on IReflect.
		else if constexpr (std::is_same<T, IReflect>::value)
		{
			return false;
		}
		else
		{
			return IsOrDescendantOf<typename T::SuperClass, TOther>();
		}
	}
}