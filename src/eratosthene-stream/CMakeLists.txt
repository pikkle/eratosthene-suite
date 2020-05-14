cmake_minimum_required(VERSION 3.15)
project(eratosthene-stream)

set(CMAKE_CXX_STANDARD 17)
find_package(glfw3 3.2 REQUIRED)
find_package(Vulkan REQUIRED)

# include head-files libraries
include_directories("include")
add_subdirectory("include/IXWebSocket")

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
        src/engine.h
        src/models.h
        src/utils.h
        src/server.h
        )

set(EXTERNAL_HEADERS
        include/stb/stb_image.h
        include/stb/stb_image_write.h
        include/base64/base64.h
        include/base64/base64.cpp
        include/nlohmann/json.hpp
        )

set(SOURCES
        src/engine.cpp
        src/server.cpp
        src/utils.cpp)


foreach (RESOURCE ${RESOURCES})
    add_custom_command(OUTPUT ${RESOURCE}.spv
            COMMAND ${GLSL_COMPILER} ${RESOURCE} -o ${RESOURCE}.spv
            DEPENDS ${RESOURCE}
            COMMENT "Rebuilding ${RESOURCE}.spv")
    message(STATUS "Generating build commands for ${RESOURCE}.spv")
endforeach ()

add_executable(
        eratosthene-stream
        ${EXTERNAL_HEADERS}
        ${HEADERS}
        ${SOURCES}
        ${RESOURCES}
        ${COMPILED_RESOURCES}
)

target_link_libraries(eratosthene-stream glfw ${GLFW_LIBRARIES} Vulkan::Vulkan ixwebsocket)