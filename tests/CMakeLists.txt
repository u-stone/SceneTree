# This file builds the test executable.

add_executable(scene_manager_tests
    scene_manager_tests.cpp
)

# Link the tests against our library and Google Test
target_link_libraries(scene_manager_tests PRIVATE
    SceneTreeLib
    gtest_main # gtest_main includes a main function
)

include(GoogleTest)

# Set the policy for test discovery to avoid build-time errors.
# NEW behavior uses PRE_TEST discovery (at test time), which avoids
# circular dependencies during the build.
if(POLICY CMP0118)
    cmake_policy(SET CMP0118 NEW)
endif()

# This creates a CTest test that runs our executable
gtest_discover_tests(scene_manager_tests)

# Set the folder for Visual Studio
set_property(TARGET scene_manager_tests PROPERTY FOLDER "Tests")

