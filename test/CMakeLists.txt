enable_testing()

set( GTest_DIR ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/gtest )
find_package( GTest CONFIG REQUIRED GTest::gtest GTest::gtest_main )

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
target_sources( ${TEST_NAME} 
	PRIVATE
		${CRG_EDITORCONFIG_FILE}
)
target_link_libraries( ${TEST_NAME}
	PUBLIC
		crg::RenderGraph
		GTest::gtest
)
target_include_directories( ${TEST_NAME}
	PUBLIC
		${VULKAN_HEADERS_INCLUDE_DIRS}
)
set_target_properties( ${TEST_NAME}
	PROPERTIES
		CXX_STANDARD 20
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
	target_sources( ${TEST_NAME} 
		PRIVATE
			${CRG_EDITORCONFIG_FILE}
	)
	target_link_libraries( ${TEST_NAME}
		PRIVATE
			crg::TestCommon
	)
	set_target_properties( ${TEST_NAME}
		PROPERTIES
			CXX_STANDARD 20
			FOLDER "Tests/${MAIN_PROJECT_NAME}"
	)
	target_compile_definitions( ${TEST_NAME}
		PRIVATE
			CRG_TestSuiteName=${TEST_NAME}
			CRG_TestSuiteNameString="${TEST_NAME}"
	)
	target_include_directories( ${TEST_NAME}
		PRIVATE
			${VULKAN_HEADERS_INCLUDE_DIRS}
	)
	target_add_coverage_flags( ${TEST_NAME} )
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
	if ( MSVC )
		target_compile_options( ${TEST_NAME} PRIVATE
			-bigobj
		)
	endif ()
	if ( PROJECTS_COVERAGE )
		coverage_add_target( ${TEST_NAME}
			MODULES
				$<TARGET_FILE_NAME:RenderGraph>
				$<TARGET_FILE_NAME:${TEST_NAME}>
			SOURCES
				${CRG_SOURCE_DIR}/include
				${CRG_SOURCE_DIR}/source
			EXCLUDES
				DotExport
		)
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

if ( PROJECTS_COVERAGE )
	coverage_add_merge_target( RenderGraphCoverage
		${PROJECTS_DOCUMENTATION_OUTPUT_DIR}/RenderGraphCoverage
	)
endif ()
