cmake_minimum_required(VERSION 3.21)
project (Reflect)
set(CMAKE_CXX_STANDARD 17)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

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
