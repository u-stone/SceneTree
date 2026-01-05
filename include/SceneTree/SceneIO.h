#pragma once

#include <string>
#include <memory>
#include "SceneTree/SceneTree.h"

class SceneIO {
public:
    // Save a SceneTree to a JSON file
    // Returns true if successful, false otherwise
    static bool saveSceneTree(const SceneTree& tree, const std::string& filepath);

    // Load a SceneTree from a JSON file
    // Returns a unique_ptr to the loaded SceneTree, or nullptr if loading failed
    static std::unique_ptr<SceneTree> loadSceneTree(const std::string& filepath);
};