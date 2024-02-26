enable_testing()
include( OpenCppCoverage )

if(OpenCppCoverage_FOUND)
	function(MyOpenCppCoverage_add_target target)
		cmake_parse_arguments(PARSE_ARGV 1 ARG "" "WORKING_DIRECTORY" "SOURCES;MODULES;OCC_ARGS;ARGS")
		if(ARG_UNPARSED_ARGUMENTS)
			message(FATAL_ERROR "Invalid argument(s): ${ARG_UNPARSED_ARGUMENTS}")
		endif()

		get_property(counter GLOBAL PROPERTY OpenCppCoverage_COUNTER)
		if(NOT counter)
			set(counter 1)
		else()
			math(EXPR counter "${counter} + 1")
		endif()
		set(outputFile ${CMAKE_CURRENT_BINARY_DIR}/OpenCppCoverage/cov-${counter}-${target}.bin)
		set_property(GLOBAL PROPERTY OpenCppCoverage_COUNTER "${counter}")
		set_property(GLOBAL APPEND PROPERTY OpenCppCoverage_SOURCES "${outputFile}")

		if(NOT ARG_WORKING_DIRECTORY)
			set(ARG_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
		endif()
		if(NOT ARG_SOURCES)
			file(TO_NATIVE_PATH ${PROJECT_SOURCE_DIR} ARG_SOURCES)
		endif()
		if(NOT ARG_MODULES)
			file(TO_NATIVE_PATH ${PROJECT_BINARY_DIR} ARG_MODULES)
		endif()

		set(args "${ARG_OCC_ARGS}")
		foreach(el IN LISTS ARG_SOURCES)
			list(APPEND args --sources ${el})
		endforeach()
		foreach(el IN LISTS ARG_MODULES)
			list(APPEND args --modules ${el})
		endforeach()
		file( TO_NATIVE_PATH "${args}" args )
		add_custom_command(TARGET ${target}
			POST_BUILD
			COMMENT "Creating coverage for ${target}"
			COMMAND ${OpenCppCoverage_BINARY}
				--working_dir $<TARGET_FILE_DIR:${target}>
				--export_type binary:${outputFile}
				--cover_children
				--quiet
				${args}
				-- $<TARGET_FILE:${target}> ${ARG_ARGS}
			VERBATIM
		)
	endfunction()
endif ()

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
	if ( PROJECTS_COVERAGE )
		MyOpenCppCoverage_add_target( ${TEST_NAME}
			MODULES
				$<TARGET_FILE_NAME:RenderGraph>
			SOURCES
				${CRG_SOURCE_DIR}/include
				${CRG_SOURCE_DIR}/source
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
	if ( PROJECTS_COVERAGE_HTML_RESULTS )
		OpenCppCoverage_add_merge_target( RenderGraphCoverage
			${PROJECTS_DOCUMENTATION_OUTPUT_DIR}/RenderGraphCoverage
			FORMAT html )
	else ()
		OpenCppCoverage_add_merge_target( RenderGraphCoverage
			${PROJECTS_DOCUMENTATION_OUTPUT_DIR}/RenderGraphCoverage.xml
			FORMAT cobertura )
	endif ()
endif ()
