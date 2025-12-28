#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include "Scene.h"
#include "SceneTree.h"

class SceneManager {
public:
    SceneManager();

    void registerScene(std::shared_ptr<Scene> scene);
    bool switchToScene(const std::string& sceneName);

    Scene* getScene(const std::string& name);
    SceneTree* getActiveSceneTree() const;
    
    // Advanced usage: Allows attaching one loaded scene to another
    bool attachScene(const std::string& parentSceneName, const std::string& childSceneName, unsigned int parentNodeId);


private:
    std::unordered_map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::unique_ptr<SceneTree> m_active_scene_tree;
};
