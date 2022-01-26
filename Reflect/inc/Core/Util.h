#pragma once

#include "Core/Core.h"
#include <string>
#include <algorithm>
#include <typeinfo>
#include <vector>
#include <map>

namespace Reflect
{
	class Class;
	class ReflectMember;

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
		std::string Demangled(const std::type_info& info);

		template<typename T>
		constexpr const char* GetTypeName()
		{
			return Demangled(typeid(T));
		}

		template<typename T>
		std::string GetTypeName(const T& type)
		{
			return ValidateTypeName(GetTypeName<T>());
		}
	}
}
