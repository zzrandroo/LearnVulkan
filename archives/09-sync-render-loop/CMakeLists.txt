cmake_minimum_required(VERSION 3.24)

project(LearnVulkanStage09 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG QUIET)

if (NOT TARGET glfw)
    include(FetchContent)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.4.zip
    )
    FetchContent_MakeAvailable(glfw)
endif()

add_executable(LearnVulkanStage09
    src/main.cpp
)

target_link_libraries(LearnVulkanStage09 PRIVATE Vulkan::Vulkan glfw)

target_compile_definitions(LearnVulkanStage09 PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

if (MSVC)
    target_compile_options(LearnVulkanStage09 PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(LearnVulkanStage09 PRIVATE -Wall -Wextra -Wpedantic)
endif()
