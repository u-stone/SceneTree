#include "gtest/gtest.h"
#include "SceneIO.h"
#include "SceneTree.h"
#include "SceneNode.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class SceneIOTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for test files
        // Using current_path to ensure write permissions in the build directory
        testDir = fs::current_path() / "SceneIOTest_Data";
        if (!fs::exists(testDir)) {
            fs::create_directory(testDir);
        }
    }

    void TearDown() override {
        // Cleanup
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    fs::path testDir;
};

TEST_F(SceneIOTest, SaveAndLoadTags) {
    // 1. Setup Tree with Tags
    auto root = std::make_shared<SceneNode>(1, "Root");
    root->addTag("LevelRoot");
    
    auto child = std::make_shared<SceneNode>(2, "Child");
    child->addTag("Enemy");
    child->addTag("Destructible");
    
    root->addChild(child);
    
    auto tree = std::make_unique<SceneTree>(root);

    // 2. Save to JSON
    fs::path filepath = testDir / "tags_test.json";
    bool saved = SceneIO::saveSceneTree(*tree, filepath.string());
    ASSERT_TRUE(saved) << "Failed to save scene tree to " << filepath;

    // 3. Load from JSON
    auto loadedTree = SceneIO::loadSceneTree(filepath.string());
    ASSERT_NE(loadedTree, nullptr) << "Failed to load scene tree from " << filepath;

    // 4. Verify Tags
    auto loadedRoot = loadedTree->getRoot();
    ASSERT_NE(loadedRoot, nullptr);
    EXPECT_EQ(loadedRoot->getId(), 1);
    EXPECT_TRUE(loadedRoot->hasTag("LevelRoot"));
    EXPECT_EQ(loadedRoot->getTags().size(), 1);

    auto loadedChild = loadedTree->findNode(2);
    ASSERT_NE(loadedChild, nullptr);
    EXPECT_TRUE(loadedChild->hasTag("Enemy"));
    EXPECT_TRUE(loadedChild->hasTag("Destructible"));
    EXPECT_EQ(loadedChild->getTags().size(), 2);
    
    // 5. Verify Tag Indexing in Loaded Tree
    auto enemies = loadedTree->findAllNodesByTag("Enemy");
    EXPECT_EQ(enemies.size(), 1);
    if (!enemies.empty()) {
        EXPECT_EQ(enemies[0]->getId(), 2);
    }
}

TEST_F(SceneIOTest, SaveAndLoadEmptyTags) {
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto tree = std::make_unique<SceneTree>(root);

    fs::path filepath = testDir / "no_tags_test.json";
    ASSERT_TRUE(SceneIO::saveSceneTree(*tree, filepath.string()));

    auto loadedTree = SceneIO::loadSceneTree(filepath.string());
    ASSERT_NE(loadedTree, nullptr);

    EXPECT_TRUE(loadedTree->getRoot()->getTags().empty());
}

TEST_F(SceneIOTest, LoadLegacyFormat) {
    // Manually create a legacy JSON file (no format_version wrapper, root object at top level)
    fs::path filepath = testDir / "legacy_test.json";
    std::ofstream ofs(filepath);
    ofs << R"({
        "id": 1,
        "name": "LegacyRoot",
        "status": "Active",
        "children": []
    })";
    ofs.close();

    auto tree = SceneIO::loadSceneTree(filepath.string());
    ASSERT_NE(tree, nullptr) << "Failed to load legacy format JSON";
    EXPECT_EQ(tree->getRoot()->getId(), 1);
    EXPECT_EQ(tree->getRoot()->getName(), "LegacyRoot");
}

TEST_F(SceneIOTest, LoadFutureVersion) {
    // Manually create a future version JSON file
    fs::path filepath = testDir / "future_version_test.json";
    std::ofstream ofs(filepath);
    ofs << R"({
        "format_version": 999,
        "root": {
            "id": 100,
            "name": "FutureRoot",
            "status": "Active"
        }
    })";
    ofs.close();

    // Capture stderr to verify the warning message
    testing::internal::CaptureStderr();
    auto tree = SceneIO::loadSceneTree(filepath.string());
    std::string output = testing::internal::GetCapturedStderr();
    
    // Verify that it loaded despite the version mismatch (forward compatibility attempt)
    ASSERT_NE(tree, nullptr);
    EXPECT_EQ(tree->getRoot()->getId(), 100);
    
    // Verify the warning was printed
    EXPECT_NE(output.find("Warning: File version (999) is newer"), std::string::npos);
}