cmake_minimum_required(VERSION 3.10)

# set the project name
project(LearnVulkan)

set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SKIP_INSTALL_RULES True)

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

message(DEBUG "Project located in: ${PROJECT_SOURCE_DIR}")

# GLFW
find_library(GLFW glfw /opt/homebrew/lib)

# Vulkan
find_library(VULKAN vulkan $ENV{VK_SDK}/lib)

# add the executable
add_executable(main.app main.cpp)

target_include_directories(main.app PUBLIC
    /opt/homebrew/include
    $ENV{VK_SDK}/include
)

target_link_libraries(main.app LINK_PUBLIC
    ${GLFW}
    ${VULKAN}
)