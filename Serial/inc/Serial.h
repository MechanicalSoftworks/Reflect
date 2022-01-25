#pragma once

#include <ostream>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#define CPP_17 1
#else
#define CPP_17 0
#endif

#if CPP_17  == 0
#pragma error C++17 must be used.
#endif

#define __SERIAL_BODY_MACRO_COMBINE_INNER(A, B, C, D) A##B##C##D
#define __SERIAL_BODY_MACRO_COMBINE(A, B, C, D) __SERIAL_BODY_MACRO_COMBINE_INNER(A, B, C, D)

#define SERIAL_GENERATED_BODY(...) __SERIAL_BODY_MACRO_COMBINE(CURRENT_FILE_ID, _, __LINE__, _SERIAL_GENERATED_BODY);

namespace Serial
{
	#define SERIAL_MAJOR 1
	#define SERIAL_MINOR 0
	#define SERIAL_PATCH 0

	constexpr const char* SerialGeneratedBodykey = "SERIAL_GENERATED_BODY";
	constexpr const char* SerialFileGeneratePrefix = ".serial";
	constexpr const char* SerialFileHeaderGuard = "_serial";

	// Reads a field from the stream into the given member.
	template<typename T, size_t offset>
	void ReadField(std::ostream& o, void* self)
	{
		o >> *(T*)((char*)self + offset);
	}
	typedef void (*ReadFieldType)(std::ostream& s, void* self);

	struct UnserialiseField
	{
		const char* name;
		ReadFieldType read;
	};
}

#define SERIAL_GET_VERSION() \
	 ((((uint32_t)(SERIAL::SERIAL_MAJOR)) << 22) | (((uint32_t)(SERIAL::SERIAL_MINOR)) << 12) | ((uint32_t)(SERIAL::SERIAL_PATCH)))

#define SERIAL_VERSION_MAJOR() ((uint32_t)(SERIAL_GET_VERSION()) >> 22)
#define SERIAL_VERSION_MINOR() (((uint32_t)(SERIAL_GET_VERSION()) >> 12) & 0x3ff)
#define SERIAL_VERSION_PATCH() ((uint32_t)(SERIAL_GET_VERSION()) & 0xfff)