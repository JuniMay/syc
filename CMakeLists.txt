cmake_minimum_required(VERSION 3.10)

project(syc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# For memory leak detection
add_compile_options(-fsanitize=address)

add_link_options(-fsanitize=address)

# Compiler executable
add_executable(syc ${SOURCES})

# Unittest executable
add_executable(syc_test ${SOURCES})
target_compile_definitions(syc_test PUBLIC -DUNITTEST)
