cmake_minimum_required(VERSION 3.15)
project(eratosthene-stream)

set(CMAKE_CXX_STANDARD 17)
find_package(glfw3 3.2 REQUIRED)
find_package(vulkan REQUIRED)

# include head-files libraries
include_directories(include)

# pre compile shaders
set(GLSL_COMPILER glslc)

set(RESOURCES
        ${CMAKE_SOURCE_DIR}/shaders/shader.vert
        ${CMAKE_SOURCE_DIR}/shaders/shader.frag
        )

set(COMPILED_RESOURCES
        ${CMAKE_SOURCE_DIR}/shaders/shader.vert.spv
        ${CMAKE_SOURCE_DIR}/shaders/shader.frag.spv
        )

set(HEADERS
        src/main.h
        src/models.h
        src/utils.h
        )

set(SOURCES
        src/main.cpp
        )


foreach (RESOURCE ${RESOURCES})
    add_custom_command(OUTPUT ${RESOURCE}.spv
            COMMAND ${GLSL_COMPILER} ${RESOURCE} -o ${RESOURCE}.spv
            DEPENDS ${RESOURCE}
            COMMENT "Rebuilding ${RESOURCE}.spv")
    message(STATUS "Generating build commands for ${RESOURCE}.spv")
endforeach ()

add_executable(
        eratosthene-stream
        ${HEADERS}
        ${SOURCES}
        ${RESOURCES}
        ${COMPILED_RESOURCES}
)

target_link_libraries(eratosthene-stream glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)