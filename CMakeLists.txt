cmake_minimum_required(VERSION 3.15)
project(SceneTree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# --- Testing ---
enable_testing()
add_subdirectory(tests)

# --- Global Settings ---
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
