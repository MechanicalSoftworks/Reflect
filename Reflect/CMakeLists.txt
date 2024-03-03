add_library(Reflect STATIC
	${LIB_SOURCE_FILES}
	${LIB_HEADER_FILES}
)

target_include_directories(Reflect
    PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${LIB_INCLUDE_DIR}>
	PRIVATE
        ${LIB_INCLUDE_DIR}
)

install(TARGETS Reflect)
install(DIRECTORY "${LIB_INCLUDE_DIR}/"
        DESTINATION "include"
        FILES_MATCHING
        PATTERN "*.*"
)