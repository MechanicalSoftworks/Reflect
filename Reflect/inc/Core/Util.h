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
#include <optional>

namespace Reflect
{
	struct IReflect;
	class Class;

	namespace Util
	{
		// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
		template<size_t N>
		struct StringLiteral
		{
			constexpr StringLiteral(const char(&str)[N])
			{
				std::copy_n(str, N, value);
			}

			constexpr StringLiteral(std::string_view s)
			{
				std::copy_n(s.data(), N, value);
				value[N - 1] = 0;
			}

			template<size_t N0, size_t N1>
			constexpr StringLiteral(const StringLiteral<N0>& s0, const StringLiteral<N1>& s1)
			{
				// Subtract NULL terminator.
				static_assert(N0 + N1 - 1 == N);
				std::copy_n(s0.value, N0 - 1, value);
				std::copy_n(s1.value, N1, value + N0 - 1);
			}

			constexpr operator std::string()      const { return std::string(value); }
			constexpr operator std::string_view() const { return std::string_view(value); }

			template<size_t N1> constexpr auto operator+(const StringLiteral<N1>& rhs) const { return StringLiteral<N + N1 - 1>(*this, rhs); }	// Subtract NULL terminator.
			template<size_t N1> constexpr auto operator+(const char(&rhs)[N1]) const         { return *this + StringLiteral<N1>(rhs); }

			template<size_t N1> constexpr auto operator==(const StringLiteral<N1>& rhs) const { return N == N1 && (std::string_view)*this == (std::string_view)rhs; }
			template<size_t N1> constexpr auto operator!=(const StringLiteral<N1>& rhs) const { return !(*this == rhs); }

			constexpr auto operator==(const std::string_view& rhs) const { return (std::string_view)*this == rhs; }
			constexpr auto operator!=(const std::string_view& rhs) const { return (std::string_view)*this != rhs; }

			template<size_t N1> constexpr auto operator==(const char(&rhs)[N1]) { return N == N1 && std::equal(value, value + N, rhs); }
			template<size_t N1> constexpr auto operator!=(const char(&rhs)[N1]) { return !(*this == rhs); }
		
			char value[N];
		};

		template<size_t N>
		std::ostream& operator<<(std::ostream& os, const StringLiteral<N>& s) { return os << s.value; }

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
		constexpr std::vector<T> make_vector(Args&&... args)
		{
			std::vector<T> vec;
			vec.reserve(sizeof...(Args));
			using arr_t = int[];
			(void) arr_t{0, (vec.emplace_back(std::forward<Args>(args)), 0)...};
			return vec;
		}

		constexpr static std::string ToLower(std::string str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](char c)
			{
				return static_cast<char>(std::tolower(static_cast<int>(c)));
			});
			return str;
		}

		constexpr static auto TryGetPropertyValue(const std::vector<std::string>& properties, const std::string_view& flag) -> std::optional<std::string_view>
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

				return std::optional<std::string_view>(std::in_place, p.begin() + assign + 1, p.end());
			}

			return {};
		}

		static constexpr bool ContainsProperty(const std::vector<std::string>& properties, std::vector<std::string> const& flags)
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
#	error Unsupported compiler
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

				template <auto FunctionPtr>
				constexpr auto function_name_array()
				{
#if defined(__clang__)
					constexpr auto prefix = std::string_view{ "[FunctionPtr = " };
					constexpr auto suffix = std::string_view{ "]" };
					constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(__GNUC__)
					constexpr auto prefix = std::string_view{ "with FunctionPtr = " };
					constexpr auto suffix = std::string_view{ "]" };
					constexpr auto function = std::string_view{ __PRETTY_FUNCTION__ };
#elif defined(_MSC_VER)
					constexpr auto prefix = std::string_view{ "function_name_array<" };
					constexpr auto suffix = std::string_view{ ">(void)" };
					constexpr auto function = std::string_view{ __FUNCSIG__ };
#else
#	error Unsupported compiler
#endif

					constexpr auto start = function.find(prefix) + prefix.size();
					constexpr auto end = function.rfind(suffix);

					static_assert(start < end);

					constexpr auto name = function.substr(start, (end - start));

					// Visual C++ includes the return type and parameter list.
					// We need to strip that off to make it the same as G++.
#if defined(_MSC_VER)
					// Chop off everything after the parameter list.
					constexpr auto parameter_list_start = name.rfind("(");

					// Chop off everything before the name token.
					constexpr auto is_operator = name.find("::operator ") != std::string::npos;
					constexpr auto name_start = name.rfind(" ", is_operator 
						// Subtract 4 from parameter_list_start because MSVC adds a space like this: "C::operator ()".
						? parameter_list_start - 4
						: parameter_list_start
					) + 1;

					constexpr auto actual_name = name.substr(name_start, (parameter_list_start - name_start));
					return substring_as_array(actual_name, std::make_index_sequence<actual_name.size()>{});
#else
					return substring_as_array(name, std::make_index_sequence<name.size()>{});
#endif
				}

				template <auto FunctionPtr>
				struct function_name_holder {
					static inline constexpr auto value = function_name_array<FunctionPtr>();
				};

				template <size_t N>
				struct fixed_string {
					char data[N];
					size_t size;
				};

				template <fixed_string s>
				constexpr auto add_null_terminator() {
					fixed_string<s.size + 1> r;
					
					r.size = s.size;
					std::copy_n(s.data, s.size, r.data);
					r.data[s.size] = 0;

					return r;
				}

				// MSVC specifies "class std::string", whereas GCC specifies "std::string".
				// Strip off "class ", "struct " and "enum " for MSVC to make them the same.
				template <size_t N>
				constexpr auto clean_type_name(const std::array<char, N>& expr) {
					fixed_string<N> result = {};

					size_t src_idx = 0;
					size_t dst_idx = 0;
					while (src_idx < N) {
						if (expr[src_idx] == ',' && src_idx < N - 1 && expr[src_idx + 1] == ' ') {
							result.data[dst_idx++] = ',';
							src_idx += 2;
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

				template <size_t N>
				constexpr auto clean_function_name(const std::array<char, N>& expr) {
					fixed_string<N> result = {};

					size_t src_idx = 0;
					size_t dst_idx = 0;
					while (src_idx < N) {
						if (expr[src_idx] == ' ') {
							++src_idx;
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
			constexpr auto type_name()
			{
				constexpr auto& arr = impl::type_name_holder<T>::value;
				constexpr auto c = impl::clean_type_name(arr);
				constexpr auto str = impl::add_null_terminator<c>();
				return StringLiteral{ str.data };
			}

			template <auto FunctionPtr>
			constexpr auto function_name()
			{
				constexpr auto& arr = impl::function_name_holder<FunctionPtr>::value;
				constexpr auto c = impl::clean_function_name(arr);
				constexpr auto str = impl::add_null_terminator<c>();
				return StringLiteral{ str.data };
			}

			template <typename T>
			struct TypeNameImpl {
				static constexpr inline auto value = type_name<T>();
			};

			template <> struct TypeNameImpl<bool>               { static constexpr inline auto value = StringLiteral{ "bool" }; };
			
			template <> struct TypeNameImpl<char>               { static constexpr inline auto value = StringLiteral{ "int8" }; };
			template <> struct TypeNameImpl<unsigned char>      { static constexpr inline auto value = StringLiteral{ "uint8" }; };

			template <> struct TypeNameImpl<short>              { static constexpr inline auto value = StringLiteral{ "int16" }; };
			template <> struct TypeNameImpl<unsigned short>     { static constexpr inline auto value = StringLiteral{ "uint16" }; };

			template <> struct TypeNameImpl<int>                { static constexpr inline auto value = StringLiteral{ "int32" }; };
			template <> struct TypeNameImpl<unsigned int>       { static constexpr inline auto value = StringLiteral{ "uint32" }; };

			template <> struct TypeNameImpl<long>               { static constexpr inline auto value = StringLiteral{ "int32" }; };
			template <> struct TypeNameImpl<unsigned long>      { static constexpr inline auto value = StringLiteral{ "uint32" }; };

			template <> struct TypeNameImpl<long long>          { static constexpr inline auto value = StringLiteral{ "int64" }; };
			template <> struct TypeNameImpl<unsigned long long> { static constexpr inline auto value = StringLiteral{ "uint64" }; };

			template <> struct TypeNameImpl<float>              { static constexpr inline auto value = StringLiteral{ "float32" }; };
			template <> struct TypeNameImpl<double>             { static constexpr inline auto value = StringLiteral{ "float64" }; };

			template <>
			struct TypeNameImpl<std::string>                    { static constexpr inline auto value = StringLiteral{ "std::string" }; };

			template <typename T>
			struct TypeNameImpl<std::unique_ptr<T>>             { static constexpr inline auto value = StringLiteral{ "std::unique_ptr<" } + TypeNameImpl<T>::value + ">"; };

			template <typename T>
			struct TypeNameImpl<std::shared_ptr<T>>             { static constexpr inline auto value = StringLiteral{ "std::shared_ptr<" } + TypeNameImpl<T>::value + ">"; };

			template <typename T>
			struct TypeNameImpl<std::weak_ptr<T>>               { static constexpr inline auto value = StringLiteral{ "std::weak_ptr<" }   + TypeNameImpl<T>::value + ">"; };

			template <typename T>
			struct TypeNameImpl<std::vector<T>>                 { static constexpr inline auto value = StringLiteral{ "std::vector<" }     + TypeNameImpl<T>::value + ">"; };

			template <typename T>
			struct TypeNameImpl<std::set<T>>                    { static constexpr inline auto value = StringLiteral{ "std::set<" }        + TypeNameImpl<T>::value + ">"; };

			template <typename K, typename V>
			struct TypeNameImpl<std::map<K, V>>                 { static constexpr inline auto value = StringLiteral{ "std::map<" }        + TypeNameImpl<K>::value + "," + TypeNameImpl<V>::value + ">"; };

			template <typename T>
			struct TypeNameImpl<std::atomic<T>>                 { static constexpr inline auto value = StringLiteral{ "std::atomic<" }     + TypeNameImpl<T>::value + ">"; };

			template <typename T>
			struct TypeNameImpl<T*>                             { static constexpr inline auto value = TypeNameImpl<T>::value + "*"; };

			template<typename TFirst>
			struct CallableSignatureStringBuilder;

			template<>
			struct CallableSignatureStringBuilder<std::tuple<>>
			{
				static constexpr inline StringLiteral value = "";
			};

			template<typename TFirst>
			struct CallableSignatureStringBuilder<std::tuple<TFirst>>
			{
				static constexpr inline StringLiteral value = TypeNameImpl<TFirst>::value;
			};

			template<typename TFirst, typename... TRest>
			struct CallableSignatureStringBuilder<std::tuple<TFirst, TRest...>>
			{
				static constexpr inline StringLiteral value = TypeNameImpl<TFirst>::value + "," + CallableSignatureStringBuilder<std::tuple<TRest...>>::value;
			};
		}

		template<typename TFunction> class callable_traits;

		// specialization for functions
		template<typename R, typename... TArgs>
		struct callable_traits<R(TArgs...)>
		{
			using return_type = R;
			using arguments = std::tuple<TArgs...>;
		};

		// specialization for methods
		template<typename R, typename Obj, typename... TArgs>
		struct callable_traits<R(Obj::*)(TArgs...)>
		{
			using return_type = R;
			using object_type = Obj;
			using arguments = std::tuple<TArgs...>;
		};

		template<typename T>
		constexpr auto GetTypeName()
		{
			return detail::TypeNameImpl<T>::value;
		}

		template<typename T>
		constexpr auto GetTypeName(const T& type)
		{
			return GetTypeName<T>();
		}

		template<auto FunctionPtr>
		constexpr auto GetFunctionName()
		{
			return detail::function_name<FunctionPtr>();
		}

		template<auto FunctionPtr>
		constexpr auto GetFunctionDeclaration()
		{
			using FunctionTraits = callable_traits<decltype(FunctionPtr)>;

			return GetTypeName<typename FunctionTraits::return_type>() + " " + GetFunctionName<FunctionPtr>() + "(" + detail::CallableSignatureStringBuilder<typename FunctionTraits::arguments>::value + ")";
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

		template<class T, class U> struct match_const { using type = U; };
		template<class T, class U> struct match_const<const T, U> { using type = const U; };

		[[nodiscard]] static std::string replace_all(const std::string_view& s, const std::string_view& pattern, const std::string_view& replacement)
		{
			std::string str(s);

			// https://stackoverflow.com/a/4643526
			size_t index = 0;
			while (true) {
				/* Locate the substring to replace. */
				index = str.find(pattern, index);
				if (index == std::string::npos) break;

				/* Make the replacement. */
				str.replace(index, pattern.length(), replacement);

				/* Advance index forward so the next iteration doesn't pick it up as well. */
				index += replacement.length();
			}

			return str;
		}
	}
}
