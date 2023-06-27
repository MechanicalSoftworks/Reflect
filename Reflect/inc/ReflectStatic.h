#pragma once

#include "Core/Core.h"
#include "Core/Util.h"

namespace Reflect
{
	template<typename T> struct ReflectStatic;
	struct IReflect;

	template<typename T> concept IsReflected = sizeof(ReflectStatic<typename std::decay<T>::type>) != 0;

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
		const auto& Get(const T& t) const
		{
			return *reinterpret_cast<const Type*>(reinterpret_cast<const char*>(&t) + Offset);
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
		template<typename T, int I, Util::StringLiteral... flags>
		struct FilterProperties;

		template<typename T, Util::StringLiteral... flags>
		struct FilterProperties<T, 0, flags...>
		{
			static constexpr auto call()
			{
				return std::make_tuple();
			}
		};

		template<typename T, int I, Util::StringLiteral... flags>
		struct FilterProperties
		{
			static constexpr auto call()
			{
				constexpr auto properties = ReflectStatic<T>::Properties;
				if constexpr (std::get<I - 1>(ReflectStatic<T>::Properties).template HasAnyFlag<flags...>())
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

	template<typename T, Util::StringLiteral... flags>
		requires IsReflected<T>
	constexpr auto FilterProperties()
	{
		using TDecay = typename std::decay<T>::type;

		constexpr auto properties = ReflectStatic<TDecay>::Properties;

		constexpr auto property_count = std::tuple_size<decltype(properties)>::value;
		return detail::FilterProperties<TDecay, property_count, flags...>::call();
	}

	//
	// This version has no filter.
	//
	template<typename T>
	void ForEachProperty(T& t, auto&& fn)
	{
		using TDecay = typename std::decay<T>::type;
		
		// https://stackoverflow.com/a/54053084
		std::apply(
			[&t, &fn](auto&&... args) {
				((fn(args, args.Get(t)), ...));
			},
			ReflectStatic<T>::Properties
		);

		if constexpr (!std::is_same_v<typename TDecay::SuperClass, IReflect>)
		{
			// This propgates constness from T to T::SuperClass;
			using TValue = std::remove_reference_t<T>;
			using TSuper = Util::match_const<TValue, typename TDecay::SuperClass>::type;

			ForEachProperty(static_cast<TSuper&>(t), std::move(fn));
		}
	}

	//
	// This version has no object.
	//
	template<typename T>
	void ForEachProperty(auto&& fn)
	{
		using TDecay = typename std::decay<T>::type;

		// https://stackoverflow.com/a/54053084
		std::apply(
			[&fn](auto&&... args) {
				((fn(args), ...));
			},
			ReflectStatic<T>::Properties
		);

		if constexpr (!std::is_same_v<typename TDecay::SuperClass, IReflect>)
		{
			// This propgates constness from T to T::SuperClass;
			using TValue = std::remove_reference_t<T>;
			using TSuper = Util::match_const<TValue, typename TDecay::SuperClass>::type;

			ForEachProperty<TSuper>(std::move(fn));
		}
	}

	//
	// Filter properties based on attributes.
	//
	template<typename T, Util::StringLiteral... flags>
		requires Util::StringLiteralList<flags...>&& IsReflected<T>
	void ForEachProperty(T& t, auto&& fn)
	{
		using TDecay = typename std::decay<T>::type;

		constexpr auto properties = FilterProperties<T, flags...>();

		// https://stackoverflow.com/a/54053084
		std::apply(
			[&t, &fn](auto&&... args) {
				((fn(args, args.Get(t)), ...));
			},
			properties
		);

		if constexpr (!std::is_same_v<typename TDecay::SuperClass, IReflect>)
		{
			// This propgates constness from T to T::SuperClass;
			using TValue = std::remove_reference_t<T>;
			using TSuper = Util::match_const<TValue, typename TDecay::SuperClass>::type;

			ForEachProperty<TSuper, flags...>(static_cast<TSuper&>(t), std::move(fn));
		}
	}

	//
	// Filter properties based on attributes (no object).
	//
	template<typename T, Util::StringLiteral... flags>
		requires Util::StringLiteralList<flags...>&& IsReflected<T>
	void ForEachProperty(auto&& fn)
	{
		using TDecay = typename std::decay<T>::type;

		constexpr auto properties = FilterProperties<T, flags...>();

		// https://stackoverflow.com/a/54053084
		std::apply(
			[&fn](auto&&... args) {
				((fn(args), ...));
			},
			properties
		);

		if constexpr (!std::is_same_v<typename TDecay::SuperClass, IReflect>)
		{
			// This propgates constness from T to T::SuperClass;
			using TValue = std::remove_reference_t<T>;
			using TSuper = Util::match_const<TValue, typename TDecay::SuperClass>::type;

			ForEachProperty<TSuper, flags...>(std::move(fn));
		}
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