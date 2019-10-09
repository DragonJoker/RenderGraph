find_package( GLFW )

if( GLFW_FOUND AND Vulkan_FOUND )
	file( GLOB EXAMPLES_FOLDERS
		${CMAKE_CURRENT_SOURCE_DIR}/
	)

	file( GLOB CHILDREN RELATIVE
		${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*-*
	)

	set( IncludeDirs
		${IncludeDirs}
		${GLFW_INCLUDE_DIR}
	)

	set( LinkLibraries
		${LinkLibraries}
		RenderGraph
		${BinLibraries}
		${GLFW_LIBRARIES}
		${Vulkan_LIBRARY}
	)

	foreach ( FOLDER_NAME ${CHILDREN} )
		if ( IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME} )
			set( TARGET_NAME Example-${FOLDER_NAME} )
			file( GLOB ${TARGET_NAME}_HEADER_FILES
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/*.hpp
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/*.h
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/*.inl
			)
			file( GLOB ${TARGET_NAME}_SOURCE_FILES
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/*.cpp
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/*.c
			)
			file( GLOB ${TARGET_NAME}_GLSL_SHADER_FILES
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.vert
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.frag
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.comp
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.geom
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.tesc
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.tese
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.glsl
			)
			file( GLOB ${TARGET_NAME}_SHADER_FILES
				${CMAKE_CURRENT_SOURCE_DIR}/Shaders/*.*
			)

			source_group( "Shader Files" FILES ${${TARGET_NAME}_SHADER_FILES} )

			set( ${TARGET_NAME}_VERSION_MAJOR 0 )
			set( ${TARGET_NAME}_VERSION_MINOR 1 )
			set( ${TARGET_NAME}_VERSION_BUILD 0 )

			add_executable( ${TARGET_NAME}
				${${TARGET_NAME}_HEADER_FILES}
				${${TARGET_NAME}_SOURCE_FILES}
				${${TARGET_NAME}_GLSL_SHADER_FILES}
			)
			target_compile_options( ${TARGET_NAME} PRIVATE
				${CompileOptions}
			)
			target_compile_definitions( ${TARGET_NAME} PRIVATE
				${CompileDefinitions}
			)
			target_include_directories( ${TARGET_NAME} PRIVATE
				${IncludeDirs}
			)
			target_link_libraries( ${TARGET_NAME} PRIVATE
				${LinkLibraries}
			)
			set_target_properties( ${TARGET_NAME} PROPERTIES
				CXX_STANDARD 17
				FOLDER "Examples"
			)
			add_test(
				NAME ${TARGET_NAME}
				COMMAND ${TARGET_NAME}
			)
		endif ()
	endforeach ()
else ()
	message( SEND_ERROR "Could not find GLFW." )
endif ()