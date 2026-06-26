cmake_minimum_required(VERSION 3.24)

project(LearnVulkanHello LANGUAGES CXX)

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

add_executable(LearnVulkanHello
    src/main.cpp
)

target_link_libraries(LearnVulkanHello PRIVATE Vulkan::Vulkan glfw)

if (MSVC)
    target_compile_options(LearnVulkanHello PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(LearnVulkanHello PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 把阶段存档加入根 CMake，方便 Rider 从项目根目录直接加载和调试。
# 每个存档目录仍然保留自己的 CMakeLists.txt，因此也可以单独打开和构建。
add_subdirectory(archives/01-vulkan-instance)
add_subdirectory(archives/02-validation-layers)
add_subdirectory(archives/03-window-surface)
add_subdirectory(archives/04-logical-device-queues)
