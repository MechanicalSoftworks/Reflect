#pragma once

#include "Core/Core.h"
#include <string>
#include <algorithm>
#include <typeinfo>
#include <vector>
#include <map>

namespace Reflect
{
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

		std::string ValidateTypeName(const std::string& str);
		REFLECT_DLL std::string Demangled(const std::type_info& info);

		template<typename T>
		std::string GetTypeName()
		{
			return Demangled(typeid(T));
		}

		template<typename T>
		std::string GetTypeName(const T& type)
		{
			return ValidateTypeName(GetTypeName<T>());
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
	}
}
