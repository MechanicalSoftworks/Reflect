#pragma once

#include "Core/Core.h"
#include "Core/Util.h"

namespace Reflect
{
	template<typename T> struct ReflectStatic;
	struct IReflect;

	template<typename T> struct IsReflected : std::false_type {};

	// NOTE: This relies on T::StaticClass instead of ReflectStatic<T>.
	// Using ReflectStatic<T> had some race conditions where calling IsReflected<T> BEFORE ReflectStatic<T>
	// was defined clean to IsReflected<T> erronously returning false.
	// T::StaticClass isn't prone to those issues.
	template<typename T> requires requires { T::StaticClass; } struct IsReflected<T> : std::true_type {};
	template<> struct IsReflected<IReflect> : std::true_type {};

	template<typename T> concept Reflected = IsReflected<T>::value;

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
		auto& Get(T&& t) const
		{
			using TT = std::remove_reference_t<decltype(t)>;
			using char_t = Util::match_const<TT, char>::type;
			using type_t = Util::match_const<TT, Type>::type;
			return *reinterpret_cast<type_t*>(reinterpret_cast<char_t*>(&t) + Offset);
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
		requires Reflected<std::decay_t<T>>
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
	void ForEachProperty(T&& t, auto&& fn)
	{
		using TDecay = typename std::decay<T>::type;

		// https://stackoverflow.com/a/54053084
		std::apply(
			[&t, &fn](auto&&... args) {
				((fn(args, args.Get(std::move(t))), ...));
			},
			ReflectStatic<TDecay>::Properties
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
		requires Util::StringLiteralList<flags...>&& Reflected<std::decay_t<T>>
	void ForEachProperty(T&& t, auto&& fn)
	{
		using TDecay = typename std::decay<T>::type;

		constexpr auto properties = FilterProperties<T, flags...>();

		// https://stackoverflow.com/a/54053084
		std::apply(
			[&t, &fn](auto&&... args) {
				((fn(args, args.Get(std::move(t))), ...));
			},
			properties
		);

		if constexpr (!std::is_same_v<typename TDecay::SuperClass, IReflect>)
		{
			// This propgates constness from T to T::SuperClass;
			using TValue = std::remove_reference_t<T>;
			using TSuper = Util::match_const<TValue, typename TDecay::SuperClass>::type;

			ForEachProperty<TSuper, flags...>(std::move(static_cast<TSuper&>(t)), std::move(fn));
		}
	}

	//
	// Filter properties based on attributes (no object).
	//
	template<typename T, Util::StringLiteral... flags>
		requires Util::StringLiteralList<flags...>&& Reflected<std::decay_t<T>>
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
	constexpr inline bool IsOrDescendantOf() requires Reflected<T>
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

	template<typename T, typename TOther> requires(!Reflected<T>)
		constexpr inline bool IsOrDescendantOf()
	{
		return false;
	}
}

namespace std
{
	namespace detail
	{
		// From Boost.
		inline auto hash_combine(std::size_t x, std::size_t y) -> std::size_t
		{
			return x ^ (y + 0x9e3779b9 + (x << 6) + (x >> 2));
		}

		template<typename TObject, size_t I>
		struct StaticFieldHasher
		{
			void operator()(const TObject& obj, std::size_t& seed)
			{
				constexpr auto properties = Reflect::ReflectStatic<TObject>::Properties;

				if constexpr (I < std::tuple_size_v<decltype(properties)>)
				{
					constexpr auto field = std::get<I>(properties);
					
					if constexpr (!field.template HasAnyFlag<"NoHash">())
					{
						std::hash<typename decltype(field)::Type> hasher;
						const auto y = hasher(field.Get(obj));
						seed = hash_combine(seed, y);
					}

					StaticFieldHasher<TObject, I + 1>{}(obj, seed);
				}
			}
		};
	}

	template<Reflect::Reflected T>
	struct hash<T>
	{
		inline size_t operator()(const T& v) const
		{
			std::size_t x = 0;
			
			detail::StaticFieldHasher<T, 0>{}(v, x);

			using TDecay = typename std::decay_t<T>;
			if constexpr (!std::is_same_v<typename TDecay::SuperClass, Reflect::IReflect>)
			{
				x = detail::hash_combine(x, hash<typename TDecay::SuperClass>{}(v));
			}

			return x;
		}
	};
}
