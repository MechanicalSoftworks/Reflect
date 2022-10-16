#include "Core/Enums.h"
#include "Core/Util.h"
#include "Core/Compiler.h"

#include <stdexcept>

namespace Reflect
{
	/// <summary>
	/// Dirty way of getting the enum string value.
	/// </summary>
	/// <typeparam name="E"></typeparam>
	/// <returns></returns>
	template <typename E, E> REFLECT_DLL std::string EnumToString()
	{
		std::string value = FUNC_SIG;
		int startIndex = static_cast<int>(value.find_last_of(',')) + 1;
		int endIndex = static_cast<int>(value.find_last_of('>'));
		value = value.substr(startIndex, endIndex - startIndex);
		return value;
	}

	ReflectFlags StringToReflectFlags(const std::string& str)
	{
		if (Util::ToLower(str) == "private" || EnumToString<ReflectFlags, Private>() == str) return ReflectFlags::Private;
		else if (Util::ToLower(str) == "public" || EnumToString<ReflectFlags, Public>() == str) return ReflectFlags::Public;
		else if (Util::ToLower(str) == "friend" || EnumToString<ReflectFlags, Friend>() == str) return ReflectFlags::Friend;
		return ReflectFlags::Invalid;
	}

	std::string ReflectReturnCodeToString(const ReflectReturnCode& code)
	{
		switch (code)
		{
			case ReflectReturnCode::SUCCESS: return "ReflectFuncReturnCode - Success";
			case ReflectReturnCode::FAILED: return "ReflectFuncReturnCode - Failed";

			case ReflectReturnCode::CAST_FAILED: return "ReflectFuncReturnCode - Cast Failed";
			case ReflectReturnCode::INVALID_FUNCTION_POINTER: return "ReflectFuncReturnCode - Invalid Function Pointer";
			case ReflectReturnCode::INVALID_MEMBER: return "ReflectFuncReturnCode - Invalid Member";

			default: throw std::runtime_error("[ReflectFuncReturnCodeToString] Missing ReflectFuncReturnCode to string conversion.");
		}
	}
}
