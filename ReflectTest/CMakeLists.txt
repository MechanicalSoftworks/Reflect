file(GLOB TEST_SOURCE_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)

add_executable(ReflectTest
	${TEST_SOURCE_FILES}
)

target_include_directories(ReflectTest
    PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${LIB_INCLUDE_DIR}>
	PRIVATE
        ${LIB_INCLUDE_DIR}
)

target_compile_definitions(ReflectTest PRIVATE -DPLATFORM_WINDOWS)
target_link_libraries(ReflectTest Reflect)

install(TARGETS ReflectTest DESTINATION ${CMAKE_INSTALL_PREFIX})