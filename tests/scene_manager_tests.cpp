#include "gtest/gtest.h"
#include "SceneManager.h"
#include "SceneIO.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class SceneManagerAsyncTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = fs::current_path() / "SceneManagerAsyncTest_Data";
        if (!fs::exists(testDir)) {
            fs::create_directory(testDir);
        }
        
        // Create a dummy scene file for testing
        sceneFile = testDir / "async_scene.json";
        std::ofstream ofs(sceneFile);
        ofs << R"({
            "format_version": 1,
            "root": {
                "id": 1,
                "name": "AsyncRoot",
                "status": "Active"
            }
        })";
        ofs.close();
    }

    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    fs::path testDir;
    fs::path sceneFile;
};

TEST_F(SceneManagerAsyncTest, PreloadAsync) {
    SceneManager manager;
    bool callbackCalled = false;
    bool successResult = false;

    manager.preloadSceneAsync("AsyncScene", sceneFile.string(), [&](const std::string& name, bool success) {
        callbackCalled = true;
        successResult = success;
    });

    // Simulate main loop: wait for async task completion and processing
    int attempts = 0;
    while (!manager.isSceneReady("AsyncScene") && attempts < 100) {
        manager.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }

    EXPECT_TRUE(manager.isSceneReady("AsyncScene"));
    EXPECT_TRUE(callbackCalled);
    EXPECT_TRUE(successResult);
}

TEST_F(SceneManagerAsyncTest, LoadAsync) {
    SceneManager manager;
    bool callbackCalled = false;

    manager.loadSceneAsync("AsyncScene", sceneFile.string(), [&](const std::string& name, bool success) {
        callbackCalled = true;
    });

    int attempts = 0;
    while (manager.getActiveSceneTree() == nullptr && attempts < 100) {
        manager.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }

    ASSERT_NE(manager.getActiveSceneTree(), nullptr);
    EXPECT_EQ(manager.getActiveSceneTree()->getRoot()->getName(), "AsyncRoot");
    EXPECT_TRUE(callbackCalled);
}

TEST_F(SceneManagerAsyncTest, UnloadAsync) {
    SceneManager manager;
    
    // Preload a scene synchronously for subsequent unloading
    manager.preloadSceneAsync("ToUnload", sceneFile.string());
    while (!manager.isSceneReady("ToUnload")) {
        manager.update();
    }

    bool callbackCalled = false;
    manager.unloadSceneAsync("ToUnload", [&](const std::string& name, bool success) {
        callbackCalled = true;
    });

    int attempts = 0;
    while (callbackCalled == false && attempts < 100) {
        manager.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }

    EXPECT_FALSE(manager.isSceneReady("ToUnload"));
    EXPECT_TRUE(callbackCalled);
}