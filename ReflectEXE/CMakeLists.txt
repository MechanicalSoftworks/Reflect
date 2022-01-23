add_executable(ReflectEXE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_include_directories(ReflectEXE
    PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${LIB_INCLUDE_DIR}}>
	PRIVATE
        ${LIB_INCLUDE_DIR}
)

target_compile_definitions(ReflectEXE PRIVATE -DPLATFORM_WINDOWS -DREFLECT_EXE)
target_link_libraries(ReflectEXE Reflect)

install(TARGETS ReflectEXE DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES 
	${LIB_INCLUDE_DIR}/*
	DESTINATION include)