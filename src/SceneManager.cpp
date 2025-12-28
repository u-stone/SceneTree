#include "SceneManager.h"
#include <stdexcept>

SceneManager::SceneManager() : m_active_scene_tree(nullptr) {}

void SceneManager::registerScene(std::shared_ptr<Scene> scene) {
    if (scene) {
        m_scenes[scene->getName()] = std::move(scene);
    }
}

bool SceneManager::switchToScene(const std::string& sceneName) {
    if (m_active_scene_name == sceneName) {
        return true;
    }

    auto it = m_scenes.find(sceneName);
    if (it == m_scenes.end()) {
        return false; // Scene not found
    }

    // Create a new SceneTree from the scene data
    std::unique_ptr<SceneTree> new_tree = SceneTree::createFromScene(*it->second);
    
    if (new_tree) {
        // The old m_active_scene_tree will be automatically deallocated
        m_active_scene_tree = std::move(new_tree);
        m_active_scene_name = sceneName;
        return true;
    }

    // If scene was empty, clear the active tree
    m_active_scene_tree.reset();
    m_active_scene_name.clear();
    return true;
}

Scene* SceneManager::getScene(const std::string& name) {
    auto it = m_scenes.find(name);
    if (it != m_scenes.end()) {
        return it->second.get();
    }
    return nullptr;
}

SceneTree* SceneManager::getActiveSceneTree() const {
    return m_active_scene_tree.get();
}

bool SceneManager::attachScene(const std::string& parentSceneName, const std::string& childSceneName, ObjectId parentNodeId) {
    if (!m_active_scene_tree || m_active_scene_tree->getRoot()->getName() != parentSceneName) {
        // This advanced operation requires the parent scene to be the active one
        if (!switchToScene(parentSceneName)) {
            return false;
        }
    }

    auto child_scene_it = m_scenes.find(childSceneName);
    if (child_scene_it == m_scenes.end()) {
        return false; // Child scene not found
    }

    SceneNode* parentNode = m_active_scene_tree->findNode(parentNodeId);
    if (!parentNode) {
        return false; // Parent node not found in the active tree
    }

    auto childTree = SceneTree::createFromScene(*child_scene_it->second);
    if (!childTree) {
        return false; // Child scene is empty
    }

    return m_active_scene_tree->attach(parentNode, std::move(childTree));
}
