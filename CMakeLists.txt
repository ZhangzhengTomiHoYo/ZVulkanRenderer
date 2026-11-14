cmake_minimum_required(VERSION 3.20)
project(ZVulkanRenderer)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SRC_LIST CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/ZVulkanRenderer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ZVulkanRenderer/*.h"
)

#fetch the vulkan sdk path from environment variable
set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})

#find the header files
include_directories(
	SYSTEM ${VULKAN_SDK_PATH}/include 
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLFW/include
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLM
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/spdlog/include
	SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty 
)

#Library search paths
link_directories(
	${VULKAN_SDK_PATH}/lib 
	${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLFW/lib-vc2026
)

#slangc slang-file -entrypoint -stage outputfile-name

set(SLANGC ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/slang/bin/slangc.exe)

#find shader file folder
set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ZVulkanRenderer/Shaders)

set(SHADER_OUTPUT_DIR ${SHADER_SRC_DIR}/spv)

file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

function(compile_slang SLANG_FILE ENTRY STAGE OUTPUT_SPV)
	add_custom_command(
		OUTPUT ${OUTPUT_SPV}
		COMMAND ${SLANGC} ${SLANG_FILE}
		-entry ${ENTRY}
		-stage ${STAGE}
		-o ${OUTPUT_SPV}
		DEPENDS ${SLANG_FILE}
	)
endfunction()

file(GLOB SLANG_FILES ${SHADER_SRC_DIR}/*.slang)

set(SPV_FILES)

foreach(SLANG_FILE ${SLANG_FILES})
	# test.slang -> test 
	get_filename_component(FILE_NAME ${SLANG_FILE} NAME_WE)

	#construct the output paths for vertex and fragment shders
	set(VERT_SPV ${SHADER_OUTPUT_DIR}/${FILE_NAME}_vert.spv)
	set(FRAG_SPV ${SHADER_OUTPUT_DIR}/${FILE_NAME}_frag.spv)

	#call the function above twice
	compile_slang(${SLANG_FILE} vertMain vertex ${VERT_SPV})
	compile_slang(${SLANG_FILE} fragMain fragment ${FRAG_SPV})

	#add all .spv files to the collection list.
	list(APPEND SPV_FILES ${VERT_SPV} ${FRAG_SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_FILES})

#Main executable
add_executable(ZVulkanRenderer ${SRC_LIST})
set_target_properties(ZVulkanRenderer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(ZVulkanRenderer PRIVATE /utf-8)
target_link_libraries(ZVulkanRenderer vulkan-1.lib glfw3.lib) 

add_dependencies(ZVulkanRenderer Shaders)

add_custom_command(TARGET ZVulkanRenderer POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${SHADER_OUTPUT_DIR}
		${CMAKE_CURRENT_BINARY_DIR}/spvs
)


