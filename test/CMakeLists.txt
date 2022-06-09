enable_testing()

set( TEST_NAME TestCommon )

set( ${TEST_NAME}_HEADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/BaseTest.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/Common.hpp
)
set( ${TEST_NAME}_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/BaseTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Common.cpp
)

add_library( ${TEST_NAME}
	STATIC
	${${TEST_NAME}_HEADER_FILES}
	${${TEST_NAME}_SOURCE_FILES}
)
add_library( crg::${TEST_NAME}
	ALIAS
		${TEST_NAME}
)
target_link_libraries( ${TEST_NAME}
	PUBLIC
		crg::RenderGraph
)
target_include_directories( ${TEST_NAME}
	PUBLIC
		${VULKAN_HEADERS_INCLUDE_DIRS}
)
set_target_properties( ${TEST_NAME}
	PROPERTIES
		CXX_STANDARD 17
		FOLDER "Tests/${MAIN_PROJECT_NAME}"
)

file( GLOB TEST_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp
)

foreach ( TEST_FILE ${TEST_FILES} )
	get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
	add_executable( ${TEST_NAME}
		${TEST_FILE}
	)
	target_link_libraries( ${TEST_NAME}
		PRIVATE
			crg::TestCommon
	)
	set_target_properties( ${TEST_NAME}
		PROPERTIES
			CXX_STANDARD 17
			FOLDER "Tests/${MAIN_PROJECT_NAME}"
	)
	target_include_directories( ${TEST_NAME}
		PRIVATE
			${VULKAN_HEADERS_INCLUDE_DIRS}
	)
	if ( NOT CRG_BUILD_STATIC )
		if ( WIN32 )
			target_link_libraries( ${TEST_NAME}
				PRIVATE
					Dbghelp
			)
		else ()
			target_link_libraries( ${TEST_NAME}
				PRIVATE
					dl
			)
		endif ()
	endif ()
	add_test(
		NAME ${TEST_NAME}
		COMMAND ${TEST_NAME}
	)
	install(
		TARGETS ${TEST_NAME}
		COMPONENT ${TEST_NAME}
		EXPORT ${TEST_NAME}
		RUNTIME DESTINATION bin
	)
endforeach ()
