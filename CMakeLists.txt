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

#--------------------------------------------------------------------------------------------------
#	Adding include dirs to include search path
#--------------------------------------------------------------------------------------------------
set( CRG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
set( CRG_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} )

if ( NOT DEFINED CRG_BUILD_STATIC )
	option( CRG_BUILD_STATIC "Build as a static library" ON )
endif ()
if ( NOT DEFINED CRG_BUILD_TESTS )
	option( CRG_BUILD_TESTS "Build RenderGraph test applications" OFF )
endif ()

if ( MSVC OR NOT "${CMAKE_BUILD_TYPE}" STREQUAL "" )
	if ( NOT DEFINED VULKAN_HEADERS_INCLUDE_DIRS )
		find_path( VULKAN_HEADERS_INCLUDE_DIRS "vulkan/vk_icd.h" )
	endif ()
	# RenderGraph library
	project( RenderGraph )
	set( ${PROJECT_NAME}_HDR_FILES
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Attachment.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/AttachmentTransition.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/CallStack.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/DotExport.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Exception.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraph.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FrameGraphPrerequisites.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FramePass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FramePassDependencies.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/FramePassTimer.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/GraphContext.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/GraphNode.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/GraphVisitor.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/Id.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/ImageData.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/ImageViewData.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnableGraph.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/WriteDescriptorSet.hpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/BuilderCommon.hpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphBuilder.hpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePassDependenciesBuilder.hpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/ResourceOptimiser.hpp
	)
	set( ${PROJECT_NAME}_SRC_FILES
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/Attachment.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/AttachmentTransition.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/BuilderCommon.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/CallStack.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/DotExport.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FrameGraph.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePass.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePassDependenciesBuilder.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FramePassTimer.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphBuilder.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphContext.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/GraphNode.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/ResourceOptimiser.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnableGraph.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePass.cpp
	)
	set( ${PROJECT_NAME}_NVS_FILES
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/FrameGraph.natvis
	)
	source_group( "Header Files" FILES ${${PROJECT_NAME}_HDR_FILES} )
	source_group( "Source Files" FILES ${${PROJECT_NAME}_SRC_FILES} )
	source_group( "Visualisation Files" FILES ${${PROJECT_NAME}_NVS_FILES} )
	set( ${PROJECT_NAME}_FOLDER_HDR_FILES
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/ImageCopy.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/PipelinePass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderPass.hpp
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}/RunnablePasses/RenderQuad.hpp
	)
	set( ${PROJECT_NAME}_FOLDER_SRC_FILES
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/ImageCopy.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/PipelinePass.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderPass.cpp
		${CRG_SOURCE_DIR}/source/${PROJECT_NAME}/RunnablePasses/RenderQuad.cpp
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
	if ( CRG_BUILD_STATIC )
		add_library( ${PROJECT_NAME} STATIC
			${${PROJECT_NAME}_HDR_FILES}
			${${PROJECT_NAME}_SRC_FILES}
			${${PROJECT_NAME}_NVS_FILES}
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
		)
		if ( WIN32 )
			target_link_libraries( ${PROJECT_NAME}
				PUBLIC
					Dbghelp
			)
		else ()
			target_link_libraries( ${PROJECT_NAME}
				PUBLIC
					dl
			)
		endif ()
	endif ()
	add_library( crg::${PROJECT_NAME}
		ALIAS
			${PROJECT_NAME}
	)
	target_compile_options( ${PROJECT_NAME}
		PUBLIC
			$<$<CXX_COMPILER_ID:MSVC>:/MP>
			$<$<CXX_COMPILER_ID:MSVC>:/Zm300>
	)
	target_compile_definitions( ${PROJECT_NAME}
		PUBLIC
			CRG_VERSION_MAJOR=${VERSION_MAJOR}
			CRG_VERSION_MINOR=${VERSION_MINOR}
			CRG_VERSION_BUILD=${VERSION_BUILD}
			$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
	)
	target_include_directories( ${PROJECT_NAME}
		PUBLIC
			$<BUILD_INTERFACE:${CRG_SOURCE_DIR}/include>
			$<BUILD_INTERFACE:${CRG_BINARY_DIR}/include>
			$<BUILD_INTERFACE:${Vulkan_INCLUDE_DIR}>
			$<INSTALL_INTERFACE:include>
		PRIVATE
			${VULKAN_HEADERS_INCLUDE_DIRS}
	)
	set_target_properties( ${PROJECT_NAME}
		PROPERTIES
			CXX_STANDARD 17
			FOLDER "${CRG_BASE_DIR}/Core"
	)
	install(
		TARGETS ${PROJECT_NAME}
		COMPONENT ${PROJECT_NAME}
		EXPORT ${PROJECT_NAME}
		RUNTIME DESTINATION bin
		ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib
	)
	target_install_dir_headers( ${PROJECT_NAME}
		${CRG_SOURCE_DIR}/include/${PROJECT_NAME}
		${PROJECT_NAME}
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
