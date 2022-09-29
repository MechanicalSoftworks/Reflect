#pragma once

#include "Core/Core.h"
#include <string>
#include <algorithm>
#include <typeinfo>
#include <vector>
#include <map>
#include <atomic>
#include <array>
#include <memory>

namespace Reflect
{
	struct IReflect;
	class Class;

	namespace Util
	{
		static std::string ToLower(std::string str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](char c)
			{
				return static_cast<char>(std::tolower(static_cast<int>(c)));
			});
			return str;
		}

		//
		// Cross platform name generator.
		//
		namespace detail
		{
			namespace impl
			{
				template <std::size_t...Idxs>
				constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
				{
					return std::array{ str[Idxs]..., '\n' };
				}

				constexpr auto type_name_start(std::string_view type)
				{
					// MSVC specifies "class std::string", whereas GCC specifies "std::string".
					// Strip off "class" and "struct" for MSVC to make them the same.
#if defined(_MSC_VER)
					if (type.starts_with("enum ")) return 5;
					if (type.starts_with("class ")) return 6;
					if (type.starts_with("struct ")) return 7;
#endif
					return 0;
				}

				template <typename T>
				constexpr auto type_name_array()
				{
#if defined(__clang__)
					constexpr auto prefix = std::string_view{ "[T = " };
					constexpr auto suffix = std::string_view{ "]" };
					constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(__GNUC__)
					constexpr auto prefix = std::string_view{ "with T = " };
					constexpr auto suffix = std::string_view{ "]" };
					constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(_MSC_VER)
					constexpr auto prefix = std::string_view{ "type_name_array<" };
					constexpr auto suffix = std::string_view{ ">(void)" };
					constexpr auto function = std::string_view{ __FUNCSIG__ };
#else
# error Unsupported compiler
#endif

					constexpr auto prefix_offset = function.find(prefix) + prefix.size();
					constexpr auto start = prefix_offset + type_name_start(function.substr(prefix_offset));
					constexpr auto end = function.rfind(suffix);

					static_assert(start < end);

					constexpr auto name = function.substr(start, (end - start));
					return substring_as_array(name, std::make_index_sequence<name.size()>{});
				}

				template <typename T>
				struct type_name_holder {
					static inline constexpr auto value = type_name_array<T>();
				};
			}

			//
			// From https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/
			//
			template <typename T>
			REFLECT_CONSTEXPR auto type_name() -> std::string
			{
				constexpr auto& value = impl::type_name_holder<T>::value;
				return std::string{ value.data(), value.size() - 1 };	// -1 to strip off the '\n' appended in 'substring_as_array'.
			}

			template <typename T>
			struct TypeNameImpl {
				static REFLECT_CONSTEXPR std::string get() { return type_name<T>(); }
			};

			template <>
			struct TypeNameImpl<std::string> {
				static REFLECT_CONSTEXPR std::string get() { return "std::string"; }
			};

			template <>
			struct TypeNameImpl<long long> {
				static REFLECT_CONSTEXPR std::string get() { return "__int64"; }
			};

			template <>
			struct TypeNameImpl<unsigned long long> {
				static REFLECT_CONSTEXPR std::string get() { return "unsigned __int64"; }
			};

			template <typename T>
			struct TypeNameImpl<std::unique_ptr<T>> {
				static REFLECT_CONSTEXPR std::string get() { return "std::unique_ptr<" + TypeNameImpl<T>::get() + ">"; }
			};

			template <typename T>
			struct TypeNameImpl<std::shared_ptr<T>> {
				static REFLECT_CONSTEXPR std::string get() { return "std::shared_ptr<" + TypeNameImpl<T>::get() + ">"; }
			};

			template <typename T>
			struct TypeNameImpl<std::weak_ptr<T>> {
				static REFLECT_CONSTEXPR std::string get() { return "std::weak_ptr<" + TypeNameImpl<T>::get() + ">"; }
			};

			template <typename T>
			struct TypeNameImpl<std::vector<T>> {
				static REFLECT_CONSTEXPR std::string get() { return "std::vector<" + TypeNameImpl<T>::get() + ">"; }
			};

			template <typename K, typename V>
			struct TypeNameImpl<std::map<K, V>> {
				static REFLECT_CONSTEXPR std::string get() { return "std::map<" + TypeNameImpl<K>::get() + "," + TypeNameImpl<V>::get() + ">"; }
			};

			template <typename T>
			struct TypeNameImpl<T*> {
				static REFLECT_CONSTEXPR std::string get() { return TypeNameImpl<T>::get() + "*"; }
			};

			template <typename T>
			struct TypeNameImpl<std::atomic<T>> {
				static REFLECT_CONSTEXPR std::string get() { return "std::atomic<" + TypeNameImpl<T>::get() + ">"; }
			};
		}

		template<typename T>
		REFLECT_CONSTEXPR std::string GetTypeName()
		{
			return detail::TypeNameImpl<T>::get();
		}

		template<typename T>
		REFLECT_CONSTEXPR std::string GetTypeName(const T& type)
		{
			return GetTypeName<T>();
		}

		// Like std::find_if, but will loop around back to our initial position.
		template<typename T, typename TPred>
		T circular_find_if(const T& current, const T& begin, const T& end, TPred pred)
		{
			auto it = std::find_if(current, end, pred);
			if (it != end)
			{
				return it;
			}

			it = std::find_if(begin, current, pred);
			if (it != current)
			{
				return it;
			}

			return end;
		}

		//
		// Cross platform name generator.
		//
		namespace detail
		{
			template<typename T>
			inline constexpr typename std::enable_if<std::is_base_of_v<IReflect, T>, const Class *>::type GetStaticClass()
			{
				return &T::StaticClass;
			}

			template<typename T>
			inline constexpr typename std::enable_if<!std::is_pointer_v<T> && !std::is_base_of_v<IReflect, T>, const Class*>::type GetStaticClass()
			{
				return nullptr;
			}

			template<typename T>
			inline constexpr typename std::enable_if<std::is_pointer_v<T>, const Class*>::type GetStaticClass()
			{
				return GetStaticClass<typename std::remove_pointer<T>::type>();
			}
		}

		template<typename T>
		constexpr const Reflect::Class* GetStaticClass()
		{
			return detail::GetStaticClass<T>();
		}
	}
}
