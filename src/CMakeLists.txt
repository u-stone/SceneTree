# This file compiles the source files into a static library.

add_library(SceneTreeLib STATIC
    Scene.cpp
    SceneNode.cpp
    SceneTree.cpp
    SceneManager.cpp
    SceneIO.cpp
    Scene.h
    SceneNode.h
    SceneTree.h
    SceneManager.h
    SceneObject.h
    SceneIO.h
)

# Make the headers available to other targets (like examples and tests)
target_include_directories(SceneTreeLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link against RapidJSON (header-only)
target_link_libraries(SceneTreeLib PUBLIC rapidjson)
