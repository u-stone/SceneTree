cmake_minimum_required(VERSION 3.15)
project(SceneTree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Address Sanitizer (ASan) Support ---
option(USE_ASAN "Enable Address Sanitizer for memory safety debugging" ON)
if(USE_ASAN)
    message(STATUS "Address Sanitizer (ASan) enabled")
    if(MSVC)
        # MSVC-specific ASan flags (requires VS 2019 16.9+)
        # /Zi is required for debug symbols, /INCREMENTAL:NO is recommended for ASan
        add_compile_options(/fsanitize=address /Zi)
        add_link_options(/fsanitize=address /INCREMENTAL:NO)
    else()
        # GCC/Clang-specific ASan flags
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# --- Subdirectories ---
add_subdirectory(src)
add_subdirectory(examples)

# --- Google Test ---
# Add external directory to module path to find FetchContent.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external")
include(FetchContent)

# Set the base directory for fetched content to be inside our 'external' dir
set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- RapidJSON ---
FetchContent_Declare(
  rapidjson
  URL https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(rapidjson)

# RapidJSON v1.1.0 does not define a CMake target, so we define one manually.
if(NOT TARGET rapidjson)
    add_library(rapidjson INTERFACE)
    target_include_directories(rapidjson INTERFACE ${rapidjson_SOURCE_DIR}/include)
endif()

# --- Testing ---
enable_testing()
add_subdirectory(tests)

# --- Global Settings ---
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
