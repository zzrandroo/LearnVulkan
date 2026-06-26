cmake_minimum_required(VERSION 3.24)

project(LearnVulkanHello LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

add_executable(LearnVulkanHello
    src/main.cpp
)

target_link_libraries(LearnVulkanHello PRIVATE Vulkan::Vulkan)

if (MSVC)
    target_compile_options(LearnVulkanHello PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(LearnVulkanHello PRIVATE -Wall -Wextra -Wpedantic)
endif()
