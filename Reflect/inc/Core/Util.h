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
#include <functional>

namespace Reflect
{
	struct IReflect;
	class Class;

	namespace Util
	{
		// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
		template<size_t N>
		struct StringLiteral {
			constexpr StringLiteral(const char(&str)[N]) {
				std::copy_n(str, N, value);
			}

			char value[N];
		};

		template <StringLiteral... Args>
		concept StringLiteralList = sizeof...(Args) > 0;

		template <typename T, T... S, typename F>
		constexpr void for_sequence(std::integer_sequence<T, S...>, F&& f) {
			using unpack_t = int[];
			(void)unpack_t {
				(static_cast<void>(f(std::integral_constant<T, S>{})), 0)..., 0
			};
		}

		// https://www.fluentcpp.com/2019/03/08/stl-algorithms-on-tuples/
		template <class Tuple, class F, std::size_t... I>
		constexpr F for_each_impl(Tuple&& t, F&& f, std::index_sequence<I...>)
		{
			return (void)std::initializer_list<int>{(std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))), 0)...}, f;
		}

		// https://www.fluentcpp.com/2019/03/08/stl-algorithms-on-tuples/
		template <class Tuple, class F>
		constexpr F for_each(Tuple&& t, F&& f)
		{
			return for_each_impl(std::forward<Tuple>(t), std::forward<F>(f),
				std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
		}

		// https://www.fluentcpp.com/2019/03/08/stl-algorithms-on-tuples/
		template<typename Tuple, typename Predicate>
		constexpr size_t find_if(Tuple&& tuple, Predicate pred)
		{
			size_t index = std::tuple_size<std::remove_reference_t<Tuple>>::value;
			size_t currentIndex = 0;
			bool found = false;
			for_each(tuple, [&](auto&& value)
				{
					if (!found && pred(value))
					{
						index = currentIndex;
						found = true;
					}
					++currentIndex;
				});
			return index;
		}

		template<typename Tuple, typename Predicate>
		constexpr bool all_of(Tuple&& tuple, Predicate pred)
		{
			return find_if(tuple, std::not_fn(pred)) == std::tuple_size<std::decay_t<Tuple>>::value;
		}

		template<typename Tuple, typename Predicate>
		constexpr bool none_of(Tuple&& tuple, Predicate pred)
		{
			return find_if(tuple, pred) == std::tuple_size<std::decay_t<Tuple>>::value;
		}

		template<typename Tuple, typename Predicate>
		constexpr bool any_of(Tuple&& tuple, Predicate pred)
		{
			return !none_of(tuple, pred);
		}

		// https://stackoverflow.com/a/54487034
		template<typename tuple_t>
		constexpr auto get_array_from_tuple(tuple_t&& tuple)
		{
			constexpr auto get_array = [](auto&& ... x) { return std::array{ std::forward<decltype(x)>(x) ... }; };
			return std::apply(get_array, std::forward<tuple_t>(tuple));
		}

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
				REFLECT_CONSTEXPR auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
				{
					return std::array{ str[Idxs]... };
				}

				template <typename T>
				REFLECT_CONSTEXPR auto type_name_array()
				{
#if defined(__clang__)
					REFLECT_CONSTEXPR auto prefix = std::string_view{ "[T = " };
					REFLECT_CONSTEXPR auto suffix = std::string_view{ "]" };
					REFLECT_CONSTEXPR auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(__GNUC__)
					REFLECT_CONSTEXPR auto prefix = std::string_view{ "with T = " };
					REFLECT_CONSTEXPR auto suffix = std::string_view{ "]" };
					REFLECT_CONSTEXPR auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(_MSC_VER)
					REFLECT_CONSTEXPR auto prefix = std::string_view{ "type_name_array<" };
					REFLECT_CONSTEXPR auto suffix = std::string_view{ ">(void)" };
					REFLECT_CONSTEXPR auto function = std::string_view{ __FUNCSIG__ };
#else
# error Unsupported compiler
#endif

					REFLECT_CONSTEXPR auto start = function.find(prefix) + prefix.size();
					REFLECT_CONSTEXPR auto end = function.rfind(suffix);

					static_assert(start < end);

					REFLECT_CONSTEXPR auto name = function.substr(start, (end - start));
					return substring_as_array(name, std::make_index_sequence<name.size()>{});
				}

				template <typename T>
				struct type_name_holder {
					static inline REFLECT_CONSTEXPR auto value = type_name_array<T>();
				};

				template <size_t N>
				struct fixed_string {
					REFLECT_CONSTEXPR std::string_view view() const { return { data, size }; }
					char data[N];
					size_t size;
				};

				// MSVC specifies "class std::string", whereas GCC specifies "std::string".
				// Strip off "class ", "struct " and "enum " for MSVC to make them the same.
				template <size_t N>
				REFLECT_CONSTEXPR auto clean_expression(const std::array<char, N>& expr) {
					fixed_string<N> result = {};

					size_t src_idx = 0;
					size_t dst_idx = 0;
					while (src_idx < N) {
						if (expr[src_idx] == ' ') {
							++src_idx;
						}
						if (expr[src_idx] == 'c' && src_idx < N - 5 && expr[src_idx + 1] == 'l' && expr[src_idx + 2] == 'a' && expr[src_idx + 3] == 's' && expr[src_idx + 4] == 's' && expr[src_idx + 5] == ' ') {
							src_idx += 6;
						}
						else if (expr[src_idx] == 's' && src_idx < N - 6 && expr[src_idx + 1] == 't' && expr[src_idx + 2] == 'r' && expr[src_idx + 3] == 'u' && expr[src_idx + 4] == 'c' && expr[src_idx + 5] == 't' && expr[src_idx + 6] == ' ') {
							src_idx += 7;
						}
						else if (expr[src_idx] == 'e' && src_idx < N - 4 && expr[src_idx + 1] == 'n' && expr[src_idx + 2] == 'u' && expr[src_idx + 3] == 'm' && expr[src_idx + 4] == ' ') {
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
				REFLECT_CONSTEXPR auto& value = impl::type_name_holder<T>::value;
				return std::string{ value.data(), value.size() };
			}

			template <typename T>
			REFLECT_CONSTEXPR auto clean_type_name() -> std::string
			{
				REFLECT_CONSTEXPR auto& arr = impl::type_name_holder<T>::value;
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
		// StaticClass member accessor.
		//
		namespace detail
		{
			template<typename T>
			inline REFLECT_CONSTEXPR typename std::enable_if<std::is_base_of_v<IReflect, T>, const Class *>::type GetStaticClass()
			{
				return &T::StaticClass;
			}

			template<typename T>
			inline REFLECT_CONSTEXPR typename std::enable_if<!std::is_pointer_v<T> && !std::is_base_of_v<IReflect, T>, const Class*>::type GetStaticClass()
			{
				return nullptr;
			}

			template<typename T>
			inline REFLECT_CONSTEXPR typename std::enable_if<std::is_pointer_v<T>, const Class*>::type GetStaticClass()
			{
				return GetStaticClass<typename std::remove_pointer<T>::type>();
			}
		}

		template<typename T>
		REFLECT_CONSTEXPR const Reflect::Class* GetStaticClass()
		{
			return detail::GetStaticClass<T>();
		}
	}
}
