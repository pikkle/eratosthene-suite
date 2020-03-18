cmake_minimum_required(VERSION 3.15)
project(eratosthene-stream)

set(CMAKE_CXX_STANDARD 17)
find_package(glfw3 3.2 REQUIRED)
find_package(vulkan REQUIRED)

# include head-files libraries
include_directories(include)

# pre compile shaders
set(GLSL_COMPILER glslc)

set(KERNELS
    ${CMAKE_SOURCE_DIR}/shaders/shader.vert
    ${CMAKE_SOURCE_DIR}/shaders/shader.frag
    )

set(COMPILED_KERNELS
        ${CMAKE_SOURCE_DIR}/shaders/shader.vert.spv
        ${CMAKE_SOURCE_DIR}/shaders/shader.frag.spv
        )

foreach(KERNEL ${KERNELS})
    add_custom_command(OUTPUT ${KERNEL}.spv
            COMMAND ${GLSL_COMPILER} ${KERNEL} -o ${KERNEL}.spv

            DEPENDS ${KERNEL}
            COMMENT "Rebuilding ${KERNEL}.spv" )
    message(STATUS "Generating build commands for ${KERNEL}.spv")
endforeach()

add_executable(
        eratosthene-stream
        src/eratosthene-stream.cpp
        ${KERNELS} ${COMPILED_KERNELS}
)

target_link_libraries(eratosthene-stream glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)