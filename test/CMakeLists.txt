enable_testing()

set( TEST_COMMON_HEADER_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Common.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/DynamicLibrary.hpp
)
set( TEST_COMMON_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/Common.cpp
)
if ( WIN32 )
	set( TEST_COMMON_SOURCE_FILES
		${TEST_COMMON_SOURCE_FILES}
		${CMAKE_CURRENT_SOURCE_DIR}/Win32/DynamicLibrary.cpp
	)
else ()
	set( TEST_COMMON_SOURCE_FILES
		${TEST_COMMON_SOURCE_FILES}
		${CMAKE_CURRENT_SOURCE_DIR}/Linux/DynamicLibrary.cpp
	)
endif ()

add_library( TestCommon
	OBJECT
	${TEST_COMMON_HEADER_FILES}
	${TEST_COMMON_SOURCE_FILES}
)
target_include_directories( TestCommon PUBLIC
	${IncludeDirs}
)
target_compile_options( TestCommon PUBLIC
	${CompileOptions}
)
target_compile_definitions( TestCommon PUBLIC
	${CompileDefinitions}
)
set_target_properties( TestCommon PROPERTIES
	CXX_STANDARD 17
	FOLDER "Tests"
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
	target_compile_options( ${TEST_NAME} PRIVATE
		${CompileOptions}
	)
	target_compile_definitions( ${TEST_NAME} PRIVATE
		${CompileDefinitions}
	)
	target_include_directories( ${TEST_NAME} PRIVATE
		${IncludeDirs}
	)
	target_link_libraries( ${TEST_NAME} PRIVATE
		RenderGraph
		${BinLibraries}
	)
	set_target_properties( ${TEST_NAME} PROPERTIES
		CXX_STANDARD 17
		FOLDER "Tests"
	)
	add_test(
		NAME ${TEST_NAME}
		COMMAND ${TEST_NAME}
	)
endforeach ()
