#pragma once

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#define CPP_17 1
#else
#define CPP_17 0
#endif

#if CPP_17  == 0
#pragma error C++17 must be used.
#endif

#ifdef REFLECT_DLL_EXPORT
#define REFLECT_DLL __declspec(dllexport)
#elif defined (REFLECT_DLL_IMPORT)
#define REFLECT_DLL __declspec(dllimport)
#else 
#define REFLECT_DLL
#endif

#define REFLECT_PROPERTY(...)

#define __SERIAL_BODY_MACRO_COMBINE_INNER(A, B, C, D) A##B##C##D
#define __SERIAL_BODY_MACRO_COMBINE(A, B, C, D) __SERIAL_BODY_MACRO_COMBINE_INNER(A, B, C, D)

#define SERIAL_GENERATED_BODY(...) __SERIAL_BODY_MACRO_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY);

namespace Serial
{
	#define SERIAL_MAJOR 1
	#define SERIAL_MINOR 0
	#define SERIAL_PATCH 0

	constexpr const char* SerialGeneratedBodykey = "SERIAL_GENERATED_BODY";
	constexpr const char* SerialFileGeneratePrefix = ".serial";
	constexpr const char* SerialFileHeaderGuard = "_serial";
}

#define SERIAL_GET_VERSION() \
	 ((((uint32_t)(SERIAL::SERIAL_MAJOR)) << 22) | (((uint32_t)(SERIAL::SERIAL_MINOR)) << 12) | ((uint32_t)(SERIAL::SERIAL_PATCH)))

#define SERIAL_VERSION_MAJOR() ((uint32_t)(SERIAL_GET_VERSION()) >> 22)
#define SERIAL_VERSION_MINOR() (((uint32_t)(SERIAL_GET_VERSION()) >> 12) & 0x3ff)
#define SERIAL_VERSION_PATCH() ((uint32_t)(SERIAL_GET_VERSION()) & 0xfff)