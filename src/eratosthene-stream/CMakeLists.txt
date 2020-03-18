cmake_minimum_required(VERSION 3.15)
project(eratosthene-stream)

set(CMAKE_CXX_STANDARD 17)
find_package(glfw3 3.2 REQUIRED)
find_package(vulkan REQUIRED)

# include libraries
include(${CMAKE_CURRENT_SOURCE_DIR}/FindEmbed.cmake)
include_directories(lib/find-embed)

# include head-files libraries
include_directories(include)
#add_library(stb include/stb_image.h)

add_executable(
        eratosthene-stream
        src/eratosthene-stream.cpp
        ${EMBED_SHADER_OUTPUTS}
)

target_link_libraries(eratosthene-stream glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)
