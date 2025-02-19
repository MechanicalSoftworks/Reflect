#include "Core/Enums.h"
#include "Core/Util.h"
#include <stdexcept>

namespace Reflect
{
	EReflectFlags StringToReflectFlags(const std::string& str)
	{
		if (Util::ToLower(str) == "private" || EnumToString<EReflectFlags, Private>() == str) return EReflectFlags::Private;
		else if (Util::ToLower(str) == "public" || EnumToString<EReflectFlags, Public>() == str) return EReflectFlags::Public;
		else if (Util::ToLower(str) == "friend" || EnumToString<EReflectFlags, Friend>() == str) return EReflectFlags::Friend;
		return EReflectFlags::Invalid;
	}

	std::string ReflectReturnCodeToString(const EReflectReturnCode& code)
	{
		switch (code)
		{
			case EReflectReturnCode::SUCCESS: return "ReflectFuncReturnCode - Success";
			case EReflectReturnCode::FAILED: return "ReflectFuncReturnCode - Failed";

			case EReflectReturnCode::CAST_FAILED: return "ReflectFuncReturnCode - Cast Failed";
			case EReflectReturnCode::INVALID_FUNCTION_POINTER: return "ReflectFuncReturnCode - Invalid Function Pointer";
			case EReflectReturnCode::INVALID_MEMBER: return "ReflectFuncReturnCode - Invalid Member";

			default: throw std::runtime_error("[ReflectFuncReturnCodeToString] Missing ReflectFuncReturnCode to string conversion.");
		}
	}
}
