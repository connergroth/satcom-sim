cmake_minimum_required(VERSION 3.20)
project(satcom_sim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/satellite.cpp
    src/ground_station.cpp
    src/link.cpp
    src/packet.cpp
    src/crc.cpp
    src/main.cpp
)

# Main executable
add_executable(satcom ${SOURCES})

# Link threads
find_package(Threads REQUIRED)
target_link_libraries(satcom PRIVATE Threads::Threads)

# Tests
enable_testing()
add_subdirectory(tests)
