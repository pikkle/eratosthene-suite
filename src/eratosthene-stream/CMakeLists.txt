cmake_minimum_required(VERSION 3.15)
project(eratosthene-stream)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

set(CMAKE_CXX_STANDARD 17)
find_package(glfw3 3.2 REQUIRED)
find_package(Vulkan REQUIRED)

include (ExternalProject)

ExternalProject_Add (libcommon
        SOURCE_DIR      "../../../lib/libcommon"
        PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/libcommon
        CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/libcommon
        CMAKE_ARGS      -DUSE_SHARED_LIBRARY:BOOL=ON
        )

ExternalProject_Get_Property (libcommon install_dir)
include_directories (${install_dir}/include)
add_library (common SHARED IMPORTED)
set_target_properties (common PROPERTIES IMPORTED_LOCATION ${install_dir}/lib/libcommon.a)

ExternalProject_Add (liberatosthene
        SOURCE_DIR      "../../../lib/liberatosthene"
        PREFIX          ${CMAKE_CURRENT_BINARY_DIR}/liberatosthene
        CMAKE_ARGS      -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/liberatosthene
        CMAKE_ARGS      -DUSE_SHARED_LIBRARY:BOOL=ON
        )

ExternalProject_Get_Property (liberatosthene install_dir)
include_directories (${install_dir}/include)
add_library (eratosthene SHARED IMPORTED)
set_target_properties (eratosthene PROPERTIES IMPORTED_LOCATION ${install_dir}/lib/liberatosthene.a)


# include head-files libraries
include_directories("include")
add_subdirectory("include/IXWebSocket")

include_directories("../eratosthene-address/src")
include_directories("../eratosthene-client/src")

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
        src/entry-server.h
        src/video-client.h
        src/video-engine.h
        src/video-streamer.h
        src/data-client.h
        src/models.h
        src/utils.h
        )

set(EXTERNAL_HEADERS
        include/stb/stb_image.h
        include/stb/stb_image_write.h
        include/base64/base64.h
        include/base64/base64.cpp
        include/nlohmann/json.hpp
        include/happly/happly.h
        )

set(SOURCES
        src/entry-server.cpp
        src/video-client.cpp
        src/data-client.cpp
        src/video-engine.cpp
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
add_dependencies(eratosthene-stream libcommon)
add_dependencies(eratosthene-stream liberatosthene)
target_link_libraries(eratosthene-stream
        common eratosthene glfw ${GLFW_LIBRARIES} Vulkan::Vulkan ixwebsocket )