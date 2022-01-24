add_executable(SerialEXE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_include_directories(SerialEXE
    PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${LIB_INCLUDE_DIR}>
	PRIVATE
        ${LIB_INCLUDE_DIR}
)

target_compile_definitions(SerialEXE PRIVATE -DPLATFORM_WINDOWS -DSERIAL_EXE)
target_link_libraries(SerialEXE Serial Reflect)

install(TARGETS SerialEXE DESTINATION ${CMAKE_INSTALL_PREFIX})