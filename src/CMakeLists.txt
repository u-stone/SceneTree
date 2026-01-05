# This file compiles the source files into a static library.

add_library(SceneTreeLib STATIC
    Scene.cpp
    SceneNode.cpp
    SceneTree.cpp
    SceneManager.cpp
    SceneIO.cpp
    SceneNodePropertyObserver.cpp
)

# Make the headers available to other targets (like examples and tests)
target_include_directories(SceneTreeLib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/SceneTree
)

# Link against nlohmann_json
target_link_libraries(SceneTreeLib PUBLIC nlohmann_json::nlohmann_json)
