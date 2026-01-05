cmake_minimum_required(VERSION 3.15)
project(SceneTree LANGUAGES CXX)

# --- CMake Policy Management ---
# A helper macro to set CMake policies if they are supported by the current CMake version
macro(set_cmake_policy_if_supported policy_id behavior)
    if(POLICY ${policy_id})
        cmake_policy(SET ${policy_id} ${behavior})
    endif()
endmacro()

set_cmake_policy_if_supported(CMP0135 NEW)
# set_cmake_policy_if_supported(CMP0167 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Address Sanitizer (ASan) Support ---
option(USE_ASAN "Enable Address Sanitizer for memory safety debugging" OFF)
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
  URL "https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip"
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- nlohmann_json ---
FetchContent_Declare(
  json
  URL "https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz"
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json)

# --- TaskEngine ---
FetchContent_Declare(
  TaskEngine
  URL "https://github.com/u-stone/TaskSysten/archive/refs/tags/v0.0.3.zip"
)
FetchContent_MakeAvailable(TaskEngine)

# --- Testing ---
enable_testing()
add_subdirectory(tests)

# --- Global Settings ---
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
