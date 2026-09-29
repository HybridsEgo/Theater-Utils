cmake_minimum_required(VERSION 3.15)
project(theater_utils LANGUAGES CXX)

if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "theater_utils targets 64-bit Windows only (MCC is x64).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(theater_utils src/main.cpp)

if(MSVC)
    target_compile_options(theater_utils PRIVATE /EHsc /W3 /utf-8)
else()
    target_compile_options(theater_utils PRIVATE -Wall)
endif()

target_link_libraries(theater_utils PRIVATE advapi32 user32)
