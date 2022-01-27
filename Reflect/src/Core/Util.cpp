#include "Core/Util.h"

namespace Reflect
{
	std::string Util::ValidateTypeName(const std::string& str)
	{
		int len = (int)str.length();
		int index = len - 1;
		while (str[index] != ' ' && str[index] != str[0])
		{
			--index;
		}

		if (index == 0)
		{
			return str;
		}
		return str.substr(0, index);
	}

	std::string Util::Demangled(const std::type_info& info)
	{
#ifdef _MSC_VER
		std::string name = info.name();
		if (name.find("class ") == 0)
		{
			name = name.substr(6);
		}
		return name;
#elif defined __GNUC__
		std::unique_ptr<char, void(*)(void*)>
			name{ abi::__cxa_demangle(info.name(), 0, 0, nullptr), std::free };
		return { name.get() };
#else
#	error "Implement this platform"
#endif
	}
}
