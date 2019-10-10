cmake_minimum_required( VERSION 3.1 )
cmake_policy( VERSION 3.1 )

#--------------------------------------------------------------------------------------------------
#	Initial configurations
#--------------------------------------------------------------------------------------------------
# Set project name, used in folders and in workspace creation
set( MAIN_PROJECT_NAME "RenderGraph" )

# Set project version numbers
set( VERSION_MAJOR 0 )
set( VERSION_MINOR 1 )
set( VERSION_BUILD 0 )
set( VERSION_YEAR 2019 )

# Used to look for external modules
set( CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/CMake
	${CMAKE_SOURCE_DIR}/CMake/Modules
	${CMAKE_SOURCE_DIR}/CMake/Toolchains
)
set( CMAKE_TEMPLATES_DIR ${CMAKE_SOURCE_DIR}/CMake/Templates )

set( CMAKE_POSITION_INDEPENDENT_CODE ON )
set( CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "The configuration types" FORCE )

# Experimental Precompiled headers support for GCC
include( PCHSupport )

# Declare the project
project( ${MAIN_PROJECT_NAME} )

include( Setup )
include( Project )
include( CompilerVersion )
include( UnitTest )
include( CompilationFlags )
include( AStyleUtils )
include( ExternalDependencies )

# Organize projects into folders
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

if ( WIN32 )
	set( BinLibraries
		Dbghelp
	)
else ()
	set( BinLibraries
		dl
	)
endif ()

find_package( Doxygen )

if( DOXYGEN_FOUND )
	message( STATUS "+ Found Doxygen (version ${DOXYGEN_VERSION})" )
endif ()

set( CompileOptions
	$<$<CXX_COMPILER_ID:MSVC>:/MP>
	$<$<CXX_COMPILER_ID:MSVC>:/Zm300>
)

set( CompileDefinitions
	CRG_VERSION_MAJOR=${VERSION_MAJOR}
	CRG_VERSION_MINOR=${VERSION_MINOR}
	CRG_VERSION_BUILD=${VERSION_BUILD}
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
)

#--------------------------------------------------------------------------------------------------
#	Adding include dirs to include search path
#--------------------------------------------------------------------------------------------------
set( IncludeDirs
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_BINARY_DIR}/include
)

option( CRG_BUILD_TESTS "Build RenderGraph test applications" OFF )
option( CRG_BUILD_EXAMPLES "Build RenderGraph example applications" OFF )

if ( MSVC OR NOT "${CMAKE_BUILD_TYPE}" STREQUAL "" )
	# RenderGraph library
	project( RenderGraph )
	file( GLOB ${PROJECT_NAME}_FOLDER_HEADER_FILES
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.h
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.hpp
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.inl
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.h
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.hpp
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.inl
	)
	file( GLOB ${PROJECT_NAME}_FOLDER_SOURCE_FILES
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.c
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.cpp
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.c
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.cpp
	)
	file( GLOB ${PROJECT_NAME}_FOLDER_NATVIS_FILES
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.natvis
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.natvis
	)
	source_group( "Header Files" FILES ${${PROJECT_NAME}_FOLDER_HEADER_FILES} )
	source_group( "Source Files" FILES ${${PROJECT_NAME}_FOLDER_SOURCE_FILES} )
	source_group( "Visualisation Files" FILES ${${PROJECT_NAME}_FOLDER_NATVIS_FILES} )
	add_library( ${PROJECT_NAME} STATIC
		${${PROJECT_NAME}_FOLDER_HEADER_FILES}
		${${PROJECT_NAME}_FOLDER_SOURCE_FILES}
		${${PROJECT_NAME}_FOLDER_NATVIS_FILES}
	)
	target_compile_options( ${PROJECT_NAME} PUBLIC
		${CompileOptions}
	)
	target_compile_definitions( ${PROJECT_NAME} PUBLIC
		${CompileDefinitions}
	)
	target_include_directories( ${PROJECT_NAME} PUBLIC
		${IncludeDirs}
		${VULKAN_INCLUDE_DIR}
	)
	set_target_properties( ${PROJECT_NAME} PROPERTIES
		CXX_STANDARD 17
		FOLDER "Core"
	)
	set( LinkLibraries
		RenderGraph
	)
	install(
		TARGETS ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}
		CONFIGURATIONS Release
		EXPORT ${PROJECT_NAME}
		RUNTIME DESTINATION bin
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
	)
	install(
		TARGETS ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}_dev
		CONFIGURATIONS RelWithDebInfo
		EXPORT ${PROJECT_NAME}
		RUNTIME DESTINATION bin/RelWithDebInfo
		ARCHIVE DESTINATION lib/RelWithDebInfo
		LIBRARY DESTINATION lib/RelWithDebInfo
	)
	install(
		TARGETS ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}_dev
		CONFIGURATIONS Debug
		EXPORT ${PROJECT_NAME}
		RUNTIME DESTINATION bin/Debug
		ARCHIVE DESTINATION lib/Debug
		LIBRARY DESTINATION lib/Debug
	)
	target_install_dir_headers( ${PROJECT_NAME}
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}
		${PROJECT_NAME}
	)

	if ( CRG_BUILD_TESTS )
		add_subdirectory( test )
	endif ()

	if ( CRG_BUILD_EXAMPLES )
		add_subdirectory( examples )
	endif ()
else()
	message( SEND_ERROR "Please select a build type (Debug or Release)" )
endif()
