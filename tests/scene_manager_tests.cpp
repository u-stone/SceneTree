#include "gtest/gtest.h"
#include "SceneTree/SceneManager.h"
#include "SceneTree/SceneIO.h"
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

TEST_F(SceneManagerAsyncTest, AsyncOperationPolling) {
    SceneManager manager;
    auto op = manager.preloadSceneAsync("AsyncScene", sceneFile.string());

    ASSERT_NE(op, nullptr);
    
    int attempts = 0;
    while (!op->IsDone() && attempts < 100) {
        manager.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }

    EXPECT_TRUE(op->IsDone());
    EXPECT_TRUE(op->GetResult());
    EXPECT_TRUE(manager.isSceneReady("AsyncScene"));
}

TEST_F(SceneManagerAsyncTest, TaskMerging) {
    SceneManager manager;
    
    // Start two requests for the same scene
    auto op1 = manager.preloadSceneAsync("MergedScene", sceneFile.string());
    auto op2 = manager.loadSceneAsync("MergedScene", sceneFile.string());

    ASSERT_NE(op1, nullptr);
    ASSERT_NE(op2, nullptr);
    
    int attempts = 0;
    while ((!op1->IsDone() || !op2->IsDone()) && attempts < 100) {
        manager.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        attempts++;
    }

    EXPECT_TRUE(op1->IsDone());
    EXPECT_TRUE(op2->IsDone());
    EXPECT_TRUE(op1->GetResult());
    EXPECT_TRUE(op2->GetResult());
    
    // Verify that the second request (loadSceneAsync) triggered the switch
    ASSERT_NE(manager.getActiveSceneTree(), nullptr);
    EXPECT_EQ(manager.getActiveSceneTree()->getRoot()->getName(), "AsyncRoot");
}