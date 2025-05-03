cmake_minimum_required(VERSION 3.31)
project(Uno)

# Enable vcpkg manifest mode
set(VCPKG_MANIFEST_MODE ON)

# Automatically use vcpkg toolchain if available
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Standard C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SFML via vcpkg (version 3.x)
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

add_executable(Uno main.cpp)
# Link SFML targets
target_link_libraries(Uno PRIVATE SFML::Graphics SFML::Window SFML::System)

# Copy the font file into the runtime output directory so the exe can load it
# Assumes arial.ttf is at the project root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
file(COPY ${CMAKE_SOURCE_DIR}/arial.ttf DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})