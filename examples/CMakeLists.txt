# This file builds the example executable.

add_executable(scene_management_example
    scene_management_example.cpp
)

# Link the example against our SceneTree library
target_link_libraries(scene_management_example PRIVATE SceneTreeLib)

# Set the folder for Visual Studio
set_property(TARGET scene_management_example PROPERTY FOLDER "Examples")
