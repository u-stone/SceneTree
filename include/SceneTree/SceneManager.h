#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <future>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include "SceneTree/Scene.h"
#include "SceneTree/SceneTree.h"

class AsyncOperation {
public:
    explicit AsyncOperation(std::future<bool> future) : m_Future(std::move(future)) {}

    // Non-blocking check to see if the operation has completed.
    bool IsDone() {
        if (m_Result.has_value()) return true;
        return m_Future.valid() && m_Future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    // Gets the result. Blocks if the operation is not yet complete.
    bool GetResult() {
        if (m_Result.has_value()) return m_Result.value();
        if (m_Future.valid()) {
            m_Result = m_Future.get();
            return m_Result.value();
        }
        return false;
    }

private:
    std::future<bool> m_Future;
    std::optional<bool> m_Result;
};

class SceneManager {
public:
    SceneManager();

    void registerScene(std::shared_ptr<Scene> scene);
    bool switchToScene(const std::string& sceneName);

    Scene* getScene(const std::string& name);
    SceneTree* getActiveSceneTree() const;
    
    // Synchronous Loading and Unloading
    bool preloadScene(const std::string& sceneName, const std::string& filepath);
    bool loadScene(const std::string& sceneName, const std::string& filepath);
    bool unloadScene(const std::string& sceneName);

    // Preloading and Async Loading
    using SceneAsyncCallback = std::function<void(const std::string& sceneName, bool success)>;
    std::shared_ptr<AsyncOperation> preloadSceneAsync(const std::string& sceneName, const std::string& filepath, SceneAsyncCallback callback = nullptr);
    std::shared_ptr<AsyncOperation> loadSceneAsync(const std::string& sceneName, const std::string& filepath, SceneAsyncCallback callback = nullptr);
    std::shared_ptr<AsyncOperation> unloadSceneAsync(const std::string& sceneName, SceneAsyncCallback callback = nullptr);

    bool isSceneReady(const std::string& sceneName) const;
    
    // Should be called once per frame to process finished loading tasks
    void update();

    // Advanced usage: Allows attaching one loaded scene to another
    bool attachScene(const std::string& parentSceneName, const std::string& childSceneName, ObjectId parentNodeId);


private:
    struct AsyncRequest {
        SceneAsyncCallback callback;
        std::promise<bool> promise;
        bool autoSwitch;
    };

    struct LoadingTask {
        std::string name;
        std::future<std::unique_ptr<SceneTree>> future;
        std::vector<AsyncRequest> requests;
    };

    struct UnloadingTask {
        std::string name;
        std::future<void> future;
        SceneAsyncCallback callback;
        std::promise<bool> promise;
    };

    std::unordered_map<std::string, std::shared_ptr<Scene>> m_scenes;
    std::vector<LoadingTask> m_loading_tasks;
    std::vector<UnloadingTask> m_unloading_tasks;
    std::unordered_map<std::string, std::unique_ptr<SceneTree>> m_preloaded_trees;

    std::unique_ptr<SceneTree> m_active_scene_tree;
    std::string m_active_scene_name;
};
