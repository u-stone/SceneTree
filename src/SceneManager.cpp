#include "SceneManager.h"
#include "SceneIO.h"
#include <stdexcept>
#include <chrono>

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

    // 1. Check if the scene is already preloaded
    auto pre_it = m_preloaded_trees.find(sceneName);
    if (pre_it != m_preloaded_trees.end()) {
        m_active_scene_tree = std::move(pre_it->second);
        m_preloaded_trees.erase(pre_it);
        m_active_scene_name = sceneName;
        return true;
    }

    // 2. Fallback to synchronous creation from registered scenes
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

bool SceneManager::preloadScene(const std::string& sceneName, const std::string& filepath) {
    if (isSceneReady(sceneName)) {
        return true;
    }

    auto tree = SceneIO::loadSceneTree(filepath);
    if (tree) {
        m_preloaded_trees[sceneName] = std::move(tree);
        return true;
    }
    return false;
}

bool SceneManager::loadScene(const std::string& sceneName, const std::string& filepath) {
    if (preloadScene(sceneName, filepath)) {
        return switchToScene(sceneName);
    }
    return false;
}

bool SceneManager::unloadScene(const std::string& sceneName) {
    if (m_active_scene_name == sceneName) {
        m_active_scene_tree.reset();
        m_active_scene_name.clear();
        return true;
    }
    auto it = m_preloaded_trees.find(sceneName);
    if (it != m_preloaded_trees.end()) {
        m_preloaded_trees.erase(it);
        return true;
    }
    return false;
}

std::shared_ptr<AsyncOperation> SceneManager::preloadSceneAsync(const std::string& sceneName, const std::string& filepath, SceneAsyncCallback callback) {
    if (isSceneReady(sceneName)) {
        if (callback) callback(sceneName, true);
        std::promise<bool> p;
        p.set_value(true);
        return std::make_shared<AsyncOperation>(p.get_future());
    }

    std::promise<bool> promise;
    auto future = promise.get_future();
    m_loading_tasks.push_back({
        sceneName,
        std::async(std::launch::async, [filepath]() {
            return SceneIO::loadSceneTree(filepath);
        }),
        std::move(callback),
        false, // autoSwitch = false for preload
        std::move(promise)
    });
    return std::make_shared<AsyncOperation>(std::move(future));
}

std::shared_ptr<AsyncOperation> SceneManager::loadSceneAsync(const std::string& sceneName, const std::string& filepath, SceneAsyncCallback callback) {
    if (isSceneReady(sceneName)) {
        bool success = switchToScene(sceneName);
        if (callback) callback(sceneName, success);
        std::promise<bool> p;
        p.set_value(success);
        return std::make_shared<AsyncOperation>(p.get_future());
    }

    std::promise<bool> promise;
    auto future = promise.get_future();
    m_loading_tasks.push_back({
        sceneName,
        std::async(std::launch::async, [filepath]() {
            return SceneIO::loadSceneTree(filepath);
        }),
        std::move(callback),
        true, // autoSwitch = true for load
        std::move(promise)
    });
    return std::make_shared<AsyncOperation>(std::move(future));
}

bool SceneManager::isSceneReady(const std::string& sceneName) const {
    return m_preloaded_trees.find(sceneName) != m_preloaded_trees.end();
}

std::shared_ptr<AsyncOperation> SceneManager::unloadSceneAsync(const std::string& sceneName, SceneAsyncCallback callback) {
    std::unique_ptr<SceneTree> treeToUnload = nullptr;

    if (m_active_scene_name == sceneName) {
        treeToUnload = std::move(m_active_scene_tree);
        m_active_scene_name.clear();
    } else {
        auto it = m_preloaded_trees.find(sceneName);
        if (it != m_preloaded_trees.end()) {
            treeToUnload = std::move(it->second);
            m_preloaded_trees.erase(it);
        }
    }

    if (treeToUnload) {
        std::promise<bool> promise;
        auto future = promise.get_future();
        m_unloading_tasks.push_back({
            sceneName,
            std::async(std::launch::async, [tree = std::move(treeToUnload)]() {
                // Destruction happens here in background thread
            }),
            std::move(callback),
            std::move(promise)
        });
        return std::make_shared<AsyncOperation>(std::move(future));
    } else {
        if (callback) callback(sceneName, false); // Scene not found in memory
        std::promise<bool> p;
        p.set_value(false);
        return std::make_shared<AsyncOperation>(p.get_future());
    }
}

void SceneManager::update() {
    // Process finished loading tasks
    for (auto it = m_loading_tasks.begin(); it != m_loading_tasks.end(); ) {
        // Check if the future is ready without blocking
        if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            bool success = false;
            try {
                auto tree = it->future.get();
                if (tree) {
                    m_preloaded_trees[it->name] = std::move(tree);
                    success = true;

                    if (it->autoSwitch) {
                        success = switchToScene(it->name);
                    }
                }
            } catch (const std::exception& e) {
                // In a real engine, you'd log this error to a proper logging system
            }

            if (it->callback) {
                it->callback(it->name, success);
            }
            it->promise.set_value(success);

            it = m_loading_tasks.erase(it);
        } else {
            ++it;
        }
    }

    // Process finished unloading tasks
    for (auto it = m_unloading_tasks.begin(); it != m_unloading_tasks.end(); ) {
        if (it->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            if (it->callback) it->callback(it->name, true);
            it->promise.set_value(true);
            it = m_unloading_tasks.erase(it);
        } else {
            ++it;
        }
    }
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
