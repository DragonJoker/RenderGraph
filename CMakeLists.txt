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
set( VERSION_YEAR 2018 )

# Used to look for external modules
set( CMAKE_MODULE_PATH
	${CMAKE_SOURCE_DIR}/CMake
	${CMAKE_SOURCE_DIR}/CMake/Modules
	${CMAKE_SOURCE_DIR}/CMake/Toolchains
)
set( CMAKE_TEMPLATES_DIR ${CMAKE_SOURCE_DIR}/CMake/Templates )

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Experimental Precompiled headers support for GCC
include( PCHSupport )

set( CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "The configuration types" FORCE )

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

#--------------------------------------------------------------------------------------------------
#	Looking for needed libraries
#--------------------------------------------------------------------------------------------------
if ( CMAKE_COMPILER_IS_GNUCXX )
	DumpCompilerVersion( ${PROJECT_NAME}_COMPILER_VERSION )
	msg_debug( "GCC version : ${${PROJECT_NAME}_COMPILER_VERSION}" )
	if ( ${${PROJECT_NAME}_COMPILER_VERSION} LESS "49" )
		message( FATAL_ERROR "Your compiler is too old (${${PROJECT_NAME}_COMPILER_VERSION}), consider an update." )
	endif ()
elseif ( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )
	DumpCompilerVersion( ${PROJECT_NAME}_COMPILER_VERSION )
	msg_debug( "clang version : ${${PROJECT_NAME}_COMPILER_VERSION}" )
elseif ( MSVC )
	if ( MSVC60 OR MSVC70 OR MSVC71 OR MSVC80 OR MSVC10 OR MSVC11 OR MSVC12 )
		message( FATAL_ERROR "Your compiler is too old, consider an update." )
	endif ()
else ()
	message( FATAL_ERROR "Unsupported compiler." )
endif ()

add_compile_options( $<$<CXX_COMPILER_ID:MSVC>:/MP> )

find_package( Doxygen )

if( DOXYGEN_FOUND )
	message( STATUS "+ Found Doxygen (version ${DOXYGEN_VERSION})" )
endif ()

#--------------------------------------------------------------------------------------------------
#	MSVC specifics
#--------------------------------------------------------------------------------------------------
if( MSVC )
	# We remove the existing /Zm flag to replace it by /Zm300
	string( REGEX REPLACE ".*(/Zm[0-9]+).*" "\\1" ZmFlag ${CMAKE_CXX_FLAGS} )
	if( (NOT "${ZmFlag}" STREQUAL "") AND (NOT "${CMAKE_CXX_FLAGS}" STREQUAL "${ZmFlag}") )
		string( REPLACE ${ZmFlag} "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
	endif()
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm300" )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

add_definitions(
 -DVERSION_MAJOR=${VERSION_MAJOR}
 -DVERSION_MINOR=${VERSION_MINOR}
 -DVERSION_BUILD=${VERSION_BUILD}
)

#--------------------------------------------------------------------------------------------------
#	Adding include dirs to include search path
#--------------------------------------------------------------------------------------------------
set( ASHES_DIR external/Ashes/source/Ashes )

set( IncludeDirs
	${CMAKE_CURRENT_SOURCE_DIR}/${ASHES_DIR}/Ashes/Src
	${CMAKE_CURRENT_BINARY_DIR}/${ASHES_DIR}/Ashes/Src
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
)

# Small macro to add subdirectory files to current target source and header files
macro( parse_subdir_files _FOLDER _GROUP )
	file( GLOB _FOLDER_HEADER_FILES
		${CMAKE_SOURCE_DIR}/include/${_FOLDER}/*.h
		${CMAKE_SOURCE_DIR}/include/${_FOLDER}/*.hpp
		${CMAKE_SOURCE_DIR}/include/${_FOLDER}/*.inl
	)
	file( GLOB _FOLDER_SOURCE_FILES
		${CMAKE_SOURCE_DIR}/source/${_FOLDER}/*.c
		${CMAKE_SOURCE_DIR}/source/${_FOLDER}/*.cpp
	)
	source_group( "Header Files\\${_GROUP}" FILES ${_FOLDER_HEADER_FILES} )
	source_group( "Source Files\\${_GROUP}" FILES ${_FOLDER_SOURCE_FILES} )
endmacro()

option( CRG_BUILD_TESTS "Build RenderGraph test applications" OFF )

if ( MSVC OR NOT "${CMAKE_BUILD_TYPE}" STREQUAL "" )
	# Build external libraries
	# zlib
	set( ZLIB_SKIP_INSTALL 1 )
	add_subdirectory( external/zlib )
	set_property( TARGET zlib PROPERTY FOLDER "External" )

	# freetype
	add_subdirectory( external/FreeType )
	set_property( TARGET FreeType PROPERTY FOLDER "External" )

	# FreeImage
	if ( "${PROJECTS_PLATFORM}" STREQUAL "x64" )
		add_definitions( -D__LP64__ )
	endif ()
	add_definitions( -DFREEIMAGE_LIB )
	add_definitions( -DOPJ_STATIC )
	add_definitions( -DLIBRAW_NODLL )
	add_subdirectory( external/FreeImage )
	set_property( TARGET FreeImage PROPERTY FOLDER "External" )

	# Ashes
	set( ASHES_BUILD_RENDERER_PLUGINS TRUE )
	#set( ASHES_STATIC_RENDERER_PLUGINS TRUE )
	add_subdirectory( ${ASHES_DIR} )
	set_property( TARGET Ashes PROPERTY FOLDER "External/Ashes" )
	set_property( TARGET D3D11Renderer PROPERTY FOLDER "External/Ashes" )
	set_property( TARGET Gl4Renderer PROPERTY FOLDER "External/Ashes" )
	set_property( TARGET Gl3Renderer PROPERTY FOLDER "External/Ashes" )
	set_property( TARGET TestRenderer PROPERTY FOLDER "External/Ashes" )
	set_property( TARGET VkRenderer PROPERTY FOLDER "External/Ashes" )

	set( BinsDependencies
		Ashes
	)

	# glslang
	set( ENABLE_GLSLANG_BINARIES FALSE CACHE BOOL "Builds glslangValidator and spirv-remap" FORCE )
	set( ENABLE_AMD_EXTENSIONS FALSE CACHE BOOL "Enables support of AMD-specific extensions" FORCE )
	set( ENABLE_NV_EXTENSIONS FALSE CACHE BOOL "Enables support of Nvidia-specific extensions" FORCE )
	set( ENABLE_HLSL FALSE CACHE BOOL "Enables HLSL input support" FORCE )
	set( BUILD_TESTING OFF )
	set( SKIP_GLSLANG_INSTALL ON )
	add_subdirectory( external/glslang )
	set_property( TARGET glslang PROPERTY FOLDER "External/glslang" )
	set_property( TARGET OGLCompiler PROPERTY FOLDER "External/glslang" )
	set_property( TARGET OSDependent PROPERTY FOLDER "External/glslang" )
	set_property( TARGET SPIRV PROPERTY FOLDER "External/glslang" )
	set_property( TARGET SPVRemapper PROPERTY FOLDER "External/glslang" )

	# Build projects
	# Utils library
	project( Utils )

	if( PROJECTS_USE_PRECOMPILED_HEADERS )
		set( _USE_PCH 1 )
	else()
		set( _USE_PCH 0 )
	endif()
	set( _HAS_XINERAMA 0 )
	if ( EXISTS "/usr/lib/X11/extensions/Xinerama.h" )
		set( _HAS_XINERAMA 1 )
	elseif ( EXISTS "/usr/include/X11/extensions/Xinerama.h" )
		set( _HAS_XINERAMA 1 )
	endif ()

	configure_file( 
		${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/config.hpp.in
		${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/config.hpp
		@ONLY
		NEWLINE_STYLE LF
	)

	file( GLOB_RECURSE ${PROJECT_NAME}_FOLDER_HEADER_FILES
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.h
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.hpp
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.inl
	)
	file( GLOB_RECURSE ${PROJECT_NAME}_FOLDER_SOURCE_FILES
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.c
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.cpp
	)

	parse_subdir_files( ${PROJECT_NAME}/Align "Align" )
	parse_subdir_files( ${PROJECT_NAME}/Config "Config" )
	parse_subdir_files( ${PROJECT_NAME}/Data "Data" )
	parse_subdir_files( ${PROJECT_NAME}/Data/MiniZip "Data\\\\MiniZip" )
	parse_subdir_files( ${PROJECT_NAME}/Design "Design" )
	parse_subdir_files( ${PROJECT_NAME}/Exception "Exception" )
	parse_subdir_files( ${PROJECT_NAME}/FileParser "FileParser" )
	parse_subdir_files( ${PROJECT_NAME}/Graphics "Graphics" )
	parse_subdir_files( ${PROJECT_NAME}/Log "Log" )
	parse_subdir_files( ${PROJECT_NAME}/Math "Math" )
	parse_subdir_files( ${PROJECT_NAME}/Miscellaneous "Miscellaneous" )
	parse_subdir_files( ${PROJECT_NAME}/Multithreading "Multithreading" )
	parse_subdir_files( ${PROJECT_NAME}/Pool "Pool" )
	parse_subdir_files( ${PROJECT_NAME}/Stream "Stream" )
	parse_subdir_files( ${PROJECT_NAME}/Platform/Win32 "Platform\\\\Win32" )
	parse_subdir_files( ${PROJECT_NAME}/Platform/Android "Platform\\\\Android" )
	parse_subdir_files( ${PROJECT_NAME}/Platform/Linux "Platform\\\\Linux" )

	file( GLOB Utils_Config_HEADER_FILES
		${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/*config*.hpp
		${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/*config*.inl
	)
	file( GLOB Utils_Config_SOURCE_FILES
		${CMAKE_CURRENT_BINARY_DIR}/source/${PROJECT_NAME}/*config*.cpp
	)
	source_group( "Header Files\\Config" FILES ${Utils_Config_HEADER_FILES} )
	source_group( "Source Files\\Config" FILES ${Utils_Config_SOURCE_FILES} )

	include_directories(
		${IncludeDirs}
		${CMAKE_SOURCE_DIR}/external/zlib
		${CMAKE_BINARY_DIR}/external/zlib
		${CMAKE_SOURCE_DIR}/external/FreeType/include
		${CMAKE_SOURCE_DIR}/external/FreeType/include/freetype
		${CMAKE_SOURCE_DIR}/external/FreeImage/Source
		${CMAKE_BINARY_DIR}/external/FreeImage/Source
	)
	if( NOT WIN32 )
		find_package( X11 )
	endif()

	add_library( ${PROJECT_NAME}
		STATIC
		${${PROJECT_NAME}_FOLDER_HEADER_FILES}
		${${PROJECT_NAME}_FOLDER_SOURCE_FILES}
	)
	set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )
	set_property( TARGET ${PROJECT_NAME} PROPERTY FOLDER "Core" )

	set( BinsDependencies
		${BinsDependencies}
		Utils
	)

	# RenderGraph library
	project( RenderGraph )
	file( GLOB ${PROJECT_NAME}_FOLDER_HEADER_FILES
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.h
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.hpp
		${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.inl
	)
	file( GLOB ${PROJECT_NAME}_FOLDER_SOURCE_FILES
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.c
		${CMAKE_SOURCE_DIR}/source/${PROJECT_NAME}/*.cpp
	)
	source_group( "Header Files" FILES ${${PROJECT_NAME}_FOLDER_HEADER_FILES} )
	source_group( "Source Files" FILES ${${PROJECT_NAME}_FOLDER_SOURCE_FILES} )

	include_directories(
		${IncludeDirs}
	)

	add_library( ${PROJECT_NAME}
		STATIC
		${${PROJECT_NAME}_FOLDER_HEADER_FILES}
		${${PROJECT_NAME}_FOLDER_SOURCE_FILES}
	)
	set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )
	set_property( TARGET ${PROJECT_NAME} PROPERTY FOLDER "Core" )

	set( BinsDependencies
		${BinsDependencies}
		RenderGraph
	)

	if ( CRG_BUILD_TESTS )
		add_subdirectory( test )
	endif ()
else()
	message( SEND_ERROR "Please select a build type (Debug or Release)" )
endif()
