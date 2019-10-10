find_package( GLFW )
find_package( Vulkan )

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
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.vert
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.frag
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.comp
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.geom
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.tesc
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.tese
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.glsl
			)
			file( GLOB ${TARGET_NAME}_SHADER_FILES
				${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_NAME}/Shaders/*.*
			)

			source_group( "Header Files" FILES ${${TARGET_NAME}_HEADER_FILES} )
			source_group( "Source Files" FILES ${${TARGET_NAME}_SOURCE_FILES} )
			source_group( "Shader Files" FILES ${${TARGET_NAME}_SHADER_FILES} )

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
			install(
				TARGETS ${TARGET_NAME}
				COMPONENT ${TARGET_NAME}
				CONFIGURATIONS Release
				EXPORT ${TARGET_NAME}
				RUNTIME DESTINATION bin
			)
			install(
				TARGETS ${TARGET_NAME}
				COMPONENT ${TARGET_NAME}_dev
				CONFIGURATIONS RelWithDebInfo
				EXPORT ${TARGET_NAME}
				RUNTIME DESTINATION bin/RelWithDebInfo
			)
			install(
				TARGETS ${TARGET_NAME}
				COMPONENT ${TARGET_NAME}_dev
				CONFIGURATIONS Debug
				EXPORT ${TARGET_NAME}
				RUNTIME DESTINATION bin/Debug
			)
		endif ()
	endforeach ()
else ()
	message( SEND_ERROR "Could not find GLFW." )
endif ()