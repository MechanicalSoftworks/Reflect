cmake_minimum_required(VERSION 3.16)
project (Reflect)
set(CMAKE_CXX_STANDARD 17)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

if (UNIX)
	# Make sure we do a build type
	set(CMAKE_BUILD_TYPE "Debug")
	if(NOT CMAKE_BUILD_TYPE)
	   set(CMAKE_BUILD_TYPE "Release")
	endif()
	message("Setting CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

	# Determine optimization level.
	if(${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
		set(OPTIMIZATION_LEVEL "-O3")
	elseif(${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
		set(OPTIMIZATION_LEVEL "-Os")
	else()
		set(OPTIMIZATION_LEVEL "-O0")
	endif()

	# Determine debug info type.
	# Default to -g to preserve original behaviour.
	if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		set(DEBUG_INFO_LEVEL "-g")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPTIMIZATION_LEVEL} ${DEBUG_INFO_LEVEL}")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPTIMIZATION_LEVEL} ${DEBUG_INFO_LEVEL}")
endif (UNIX)

set(LIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Reflect/inc)
file(GLOB LIB_SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Reflect/src/*/*.cpp)
file(GLOB LIB_HEADER_FILES CONFIGURE_DEPENDS ${LIB_INCLUDE_DIR}/*/*.h ${LIB_INCLUDE_DIR}/*.h)

#===============================================================================
# LIBRARY
#===============================================================================
add_subdirectory(Reflect)
add_subdirectory(ReflectDLL)
add_subdirectory(ReflectEXE)
add_subdirectory(ReflectTest)
