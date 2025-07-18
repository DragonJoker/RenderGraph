cmake_minimum_required( VERSION 3.10 )
cmake_policy( VERSION 3.10 )

#--------------------------------------------------------------------------------------------------
#	Initial configurations
#--------------------------------------------------------------------------------------------------
# Set project name, used in folders and in workspace creation
set( MAIN_PROJECT_NAME "RenderGraph" )

# Set project version numbers
set( VERSION_MAJOR 1 )
set( VERSION_MINOR 4 )
set( VERSION_BUILD 1 )
set( VERSION_YEAR 2024 )

set( _PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}" )
set( _PROJECT_SOVERSION "${VERSION_BUILD}" )

# Used to look for external modules
if ( NOT CMAKE_MODULE_PATH )
	set( CMAKE_MODULE_PATH
		${CMAKE_SOURCE_DIR}/CMake
		${CMAKE_SOURCE_DIR}/CMake/Modules
		${CMAKE_SOURCE_DIR}/CMake/Toolchains
	)
	set( CMAKE_TEMPLATES_DIR ${CMAKE_SOURCE_DIR}/CMake/Templates )
endif ()

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
include( Coverage )

# Organize projects into folders
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

#--------------------------------------------------------------------------------------------------
#	Adding include dirs to include search path
#--------------------------------------------------------------------------------------------------
set( CRG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set( CRG_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} )

set( CRG_EDITORCONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.editorconfig )

if ( NOT DEFINED CRG_BUILD_STATIC )
	option( CRG_BUILD_STATIC "Build as a static library" ON )
endif ()
if ( NOT DEFINED CRG_BUILD_TESTS )
	option( CRG_BUILD_TESTS "Build RenderGraph test applications" OFF )
endif ()
if ( NOT DEFINED CRG_UNITY_BUILD )
	option( CRG_UNITY_BUILD "Build RenderGraph using Unity (Jumbo) build method" OFF )
endif ()

set( PROJECTS_UNITY_BUILD ${CRG_UNITY_BUILD} )

if ( MSVC OR NOT "${CMAKE_BUILD_TYPE}" STREQUAL "" )
	if ( NOT DEFINED VULKAN_HEADERS_INCLUDE_DIRS )
		find_path( VULKAN_HEADERS_INCLUDE_DIRS "vulkan/vk_icd.h" )
	endif ()
	# RenderGraph library
	project( RenderGraph )
	set( ${PROJECT_NAME}_HDR_FILES
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Attachment.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/AttachmentTransition.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/DotExport.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Exception.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraph.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraphBase.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraphEnums.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraphFunctions.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraphPrerequisites.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraphStructs.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FramePass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FramePassGroup.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FramePassTimer.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/GraphContext.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/GraphNode.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/GraphVisitor.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Id.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/ImageData.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/ImageViewData.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/LayerLayoutStatesHandler.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Log.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RecordContext.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/ResourceHandler.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnableGraph.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Signal.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/WriteDescriptorSet.hpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphBuilder.hpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePassDependenciesBuilder.hpp
	)
	set( ${PROJECT_NAME}_SRC_FILES
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/Attachment.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/AttachmentTransition.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/DotExport.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FrameGraph.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FrameGraphPrerequisites.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePass.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePassDependenciesBuilder.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePassGroup.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePassTimer.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphBuilder.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphContext.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphNode.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/LayerLayoutStatesHandler.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/Log.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RecordContext.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/ResourceHandler.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnableGraph.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePass.cpp
	)
	set( ${PROJECT_NAME}_NVS_FILES
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FrameGraph.natvis
	)
	set( ${PROJECT_NAME}_XENUM_FILES
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/PixelFormat.enum
	)
	source_group( "Header Files" FILES ${${PROJECT_NAME}_HDR_FILES} )
	source_group( "Source Files" FILES ${${PROJECT_NAME}_SRC_FILES} )
	source_group( "Visualisation Files" FILES ${${PROJECT_NAME}_NVS_FILES} )
	source_group( "X Enum Files" FILES ${${PROJECT_NAME}_XENUM_FILES} )
	set( ${PROJECT_NAME}_FOLDER_HDR_FILES
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/BufferCopy.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/BufferToImageCopy.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/ComputePass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/GenerateMipmaps.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/ImageBlit.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/ImageCopy.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/ImageToBufferCopy.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/PipelineConfig.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/PipelineHolder.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderPass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderPassHolder.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderMesh.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderMeshConfig.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderMeshHolder.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderQuad.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderQuadConfig.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderQuadHolder.hpp
	)
	set( ${PROJECT_NAME}_FOLDER_SRC_FILES
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/BufferCopy.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/BufferToImageCopy.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/ComputePass.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/GenerateMipmaps.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/ImageBlit.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/ImageCopy.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/ImageToBufferCopy.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/PipelineHolder.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderPass.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderPassHolder.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderMesh.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderMeshHolder.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderQuad.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderQuadHolder.cpp
	)
	set( ${PROJECT_NAME}_SRC_FILES
		${${PROJECT_NAME}_SRC_FILES}
		${${PROJECT_NAME}_FOLDER_SRC_FILES}
	)
	set( ${PROJECT_NAME}_HDR_FILES
		${${PROJECT_NAME}_HDR_FILES}
		${${PROJECT_NAME}_FOLDER_HDR_FILES}
	)
	source_group( "Header Files\\RunnablePasses" FILES ${${PROJECT_NAME}_FOLDER_HDR_FILES} )
	source_group( "Source Files\\RunnablePasses" FILES ${${PROJECT_NAME}_FOLDER_SRC_FILES} )
	if ( ${PROJECTS_UNITY_BUILD} )
		file( GLOB ${PROJECT_NAME}_FOLDER_SRC_FILES
			${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir/Unity/*.cxx
		)
		source_group( "Source Files\\Unity" FILES ${${PROJECT_NAME}_FOLDER_SRC_FILES} )
	endif ()
	if ( CRG_BUILD_STATIC )
		add_library( ${PROJECT_NAME} STATIC
			${${PROJECT_NAME}_HDR_FILES}
			${${PROJECT_NAME}_SRC_FILES}
			${${PROJECT_NAME}_NVS_FILES}
			${${PROJECT_NAME}_XENUM_FILES}
		)
		target_compile_definitions( ${PROJECT_NAME}
			PUBLIC
				CRG_BUILD_STATIC
		)
	else ()
		add_library( ${PROJECT_NAME} SHARED
			${${PROJECT_NAME}_HDR_FILES}
			${${PROJECT_NAME}_SRC_FILES}
			${${PROJECT_NAME}_NVS_FILES}
			${${PROJECT_NAME}_XENUM_FILES}
		)
		set_target_properties( ${PROJECT_NAME}
			PROPERTIES
				VERSION ${_PROJECT_VERSION}
				SOVERSION ${_PROJECT_SOVERSION}
		)
		if ( WIN32 )
			target_link_libraries( ${PROJECT_NAME}
				PUBLIC
					Dbghelp
			)
		else ()
			target_link_libraries( ${PROJECT_NAME}
				PRIVATE
					dl
			)
		endif ()
	endif ()
	add_library( crg::${PROJECT_NAME}
		ALIAS
			${PROJECT_NAME}
	)
	target_add_coverage_flags( ${PROJECT_NAME} )
	target_sources( ${PROJECT_NAME} 
		PRIVATE
			${CRG_EDITORCONFIG_FILE}
	)
	target_add_compilation_flags( ${PROJECT_NAME} )
	target_compile_options( ${PROJECT_NAME}
		PUBLIC
			$<$<CXX_COMPILER_ID:MSVC>:/Zm300>
			$<$<CXX_COMPILER_ID:AppleClang>:-Wno-poison-system-directories>
			$<$<CXX_COMPILER_ID:Clang>:-Wno-poison-system-directories>
	)
	target_compile_definitions( ${PROJECT_NAME}
		PUBLIC
			CRG_VERSION_MAJOR=${VERSION_MAJOR}
			CRG_VERSION_MINOR=${VERSION_MINOR}
			CRG_VERSION_BUILD=${VERSION_BUILD}
	)
	target_include_directories( ${PROJECT_NAME}
		PUBLIC
			$<BUILD_INTERFACE:${CRG_SOURCE_DIR}/include>
			$<BUILD_INTERFACE:${CRG_BINARY_DIR}>
			$<BUILD_INTERFACE:${Vulkan_INCLUDE_DIR}>
			$<INSTALL_INTERFACE:include>
		PRIVATE
			${VULKAN_HEADERS_INCLUDE_DIRS}
	)
	set_target_properties( ${PROJECT_NAME}
		PROPERTIES
			CXX_STANDARD 20
			FOLDER "${CRG_BASE_DIR}/Core"
			DEBUG_POSTFIX "d"
			UNITY_BUILD "${PROJECTS_UNITY_BUILD}"
	)
	install(
		TARGETS ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}
		EXPORT ${PROJECT_NAME}
		RUNTIME DESTINATION bin
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
	)
	target_install_headers( ${PROJECT_NAME}
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}
	)
	install( EXPORT ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}
		FILE ${PROJECT_NAME}Config.cmake
		NAMESPACE crg::
		DESTINATION lib/cmake/${PROJECT_NAME}
	)
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file( ${PROJECT_NAME}ConfigVersion.cmake
		VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUILD}
		COMPATIBILITY AnyNewerVersion
	)

	if ( CRG_BUILD_TESTS )
		add_subdirectory( test )
	endif ()
else()
	message( SEND_ERROR "Please select a build type (Debug or Release)" )
endif()
