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
	OBJECT
	${${TEST_NAME}_HEADER_FILES}
	${${TEST_NAME}_SOURCE_FILES}
)
target_link_libraries( ${TEST_NAME}
	PUBLIC
		crg::RenderGraph
)
set_target_properties( ${TEST_NAME} PROPERTIES
	CXX_STANDARD 17
	FOLDER "Tests/${MAIN_PROJECT_NAME}"
)

file( GLOB TEST_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Test*.cpp
)

foreach ( TEST_FILE ${TEST_FILES} )
	get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
	add_executable( ${TEST_NAME}
		$<TARGET_OBJECTS:TestCommon>
		${TEST_FILE}
	)
	target_link_libraries( ${TEST_NAME}
		PRIVATE
			crg::RenderGraph
	)
	set_target_properties( ${TEST_NAME} PROPERTIES
		CXX_STANDARD 17
		FOLDER "Tests/${MAIN_PROJECT_NAME}"
	)
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
	add_test(
		NAME ${TEST_NAME}
		COMMAND ${TEST_NAME}
	)
	install(
		TARGETS ${TEST_NAME}
		COMPONENT ${TEST_NAME}
		CONFIGURATIONS Release
		EXPORT ${TEST_NAME}
		RUNTIME DESTINATION bin
	)
	install(
		TARGETS ${TEST_NAME}
		COMPONENT ${TEST_NAME}_dev
		CONFIGURATIONS RelWithDebInfo
		EXPORT ${TEST_NAME}
		RUNTIME DESTINATION bin/RelWithDebInfo
	)
	install(
		TARGETS ${TEST_NAME}
		COMPONENT ${TEST_NAME}_dev
		CONFIGURATIONS Debug
		EXPORT ${TEST_NAME}
		RUNTIME DESTINATION bin/Debug
	)
endforeach ()
