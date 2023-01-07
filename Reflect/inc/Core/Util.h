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
#include <set>

namespace Reflect
{
	struct IReflect;
	class Class;

	namespace Util
	{
		// Thanks: https://tristanbrindle.com/posts/beware-copies-initializer-list
		template <typename T, typename... Args>
		REFLECT_CONSTEXPR std::vector<T> make_vector(Args&&... args)
		{
			std::vector<T> vec;
			vec.reserve(sizeof...(Args));
			using arr_t = int[];
			(void) arr_t{0, (vec.emplace_back(std::forward<Args>(args)), 0)...};
			return vec;
		}

		REFLECT_CONSTEXPR static std::string ToLower(std::string str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](char c)
			{
				return static_cast<char>(std::tolower(static_cast<int>(c)));
			});
			return str;
		}

		REFLECT_CONSTEXPR static bool TryGetPropertyValue(const std::vector<std::string>& properties, const std::string_view& flag, std::string_view& value)
		{
			for (auto const& p : properties)
			{
				if (p.find(flag) != 0)
				{
					continue;
				}

				const auto assign = p.find('=');
				if (assign != flag.length())
				{
					continue;
				}

				value = std::string_view(p.begin() + assign + 1, p.end());
				return true;
			}

			return false;
		}

		static REFLECT_CONSTEXPR bool ContainsProperty(const std::vector<std::string>& properties, std::vector<std::string> const& flags)
		{
			for (auto const& flag : flags)
			{
				for (auto const& p : properties)
				{
					if (p == flag || (p.length() >= flag.length() && p.find(flag) == 0 && p[flag.length()] == '='))
					{
						return true;
					}
				}
			}
			return false;
		}

		[[nodiscard]] static auto		SplitStringView(const std::string_view& str, char delim)
		{
			std::vector<std::string_view> v;

			auto start = 0ull;
			auto end = str.find(delim);
			while (end != std::string::npos)
			{
				v.push_back(str.substr(start, end - start));
				start = end + 1;
				end = str.find(delim, start);
			}

			if (str.length())
			{
				v.push_back(str.substr(start, end));
			}

			return v;
		}

		[[nodiscard]] static std::string_view	ltrim_stringview(const std::string_view& str, const char* chars = "\t\n\v\f\r ")
		{
			return str.substr(str.find_first_not_of(chars));
		}

		[[nodiscard]] static std::string_view	rtrim_stringview(const std::string_view& str, const char* chars = "\t\n\v\f\r ")
		{
			return str.substr(0, str.find_last_not_of(chars) + 1);
		}

		[[nodiscard]] static std::string_view	trim_stringview(const std::string_view& str, const char* chars = "\t\n\v\f\r ")
		{
			return rtrim_stringview(ltrim_stringview(str, chars), chars);
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
					return std::array{ str[Idxs]... };
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

					constexpr auto start = function.find(prefix) + prefix.size();
					constexpr auto end = function.rfind(suffix);

					static_assert(start < end);

					constexpr auto name = function.substr(start, (end - start));
					return substring_as_array(name, std::make_index_sequence<name.size()>{});
				}

				template <typename T>
				struct type_name_holder {
					static inline constexpr auto value = type_name_array<T>();
				};

				template <size_t N>
				struct fixed_string {
					constexpr std::string_view view() const { return { data, size }; }
					char data[N];
					size_t size;
				};

				// MSVC specifies "class std::string", whereas GCC specifies "std::string".
				// Strip off "class ", "struct " and "enum " for MSVC to make them the same.
				template <size_t N>
				constexpr auto clean_expression(const std::array<char, N>& expr) {
					fixed_string<N> result = {};

					int src_idx = 0;
					int dst_idx = 0;
					while (src_idx < N) {
						if (expr[src_idx] == ' ') {
							++src_idx;
						}
						if (src_idx < N - 5 && expr[src_idx] == 'c' && expr[src_idx + 1] == 'l' && expr[src_idx + 2] == 'a' && expr[src_idx + 3] == 's' && expr[src_idx + 4] == 's' && expr[src_idx + 5] == ' ') {
							src_idx += 6;
						}
						else if (src_idx < N - 6 && expr[src_idx] == 's' && expr[src_idx + 1] == 't' && expr[src_idx + 2] == 'r' && expr[src_idx + 3] == 'u' && expr[src_idx + 4] == 'c' && expr[src_idx + 5] == 't' && expr[src_idx + 6] == ' ') {
							src_idx += 7;
						}
						else if (src_idx < N - 4 && expr[src_idx] == 'e' && expr[src_idx + 1] == 'n' && expr[src_idx + 2] == 'u' && expr[src_idx + 3] == 'm' && expr[src_idx + 4] == ' ') {
							src_idx += 5;
						}
						else {
							result.data[dst_idx++] = expr[src_idx++];
						}
					}
					result.size = dst_idx;
					return result;
				}
			}

			//
			// From https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/
			//
			template <typename T>
			REFLECT_CONSTEXPR auto type_name() -> std::string
			{
				constexpr auto& value = impl::type_name_holder<T>::value;
				return std::string{ value.data(), value.size() };
			}

			template <typename T>
			REFLECT_CONSTEXPR auto clean_type_name() -> std::string
			{
				constexpr auto& arr = impl::type_name_holder<T>::value;
				return std::string(impl::clean_expression(arr).view());
			}

			template <typename T>
			struct TypeNameImpl {
				static REFLECT_CONSTEXPR std::string get() { return clean_type_name<T>(); }
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

			template <typename T>
			struct TypeNameImpl<std::set<T>> {
				static REFLECT_CONSTEXPR std::string get() { return "std::set<" + TypeNameImpl<T>::get() + ">"; }
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
