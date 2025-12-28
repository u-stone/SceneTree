#include "gtest/gtest.h"
#include "SceneIO.h"
#include "SceneTree.h"
#include "SceneNode.h"
#include <filesystem>

TEST(SceneIOTest, SaveAndLoad) {
    // 1. Create a SceneTree manually
    auto root = std::make_shared<SceneNode>(1, "Root", ObjectStatus::Active);
    auto child1 = std::make_shared<SceneNode>(2, "Child1", ObjectStatus::Inactive);
    auto child2 = std::make_shared<SceneNode>(3, "Child2", ObjectStatus::Hidden);
    
    root->addChild(child1);
    root->addChild(child2);
    
    // Add a grandchild
    auto grandchild = std::make_shared<SceneNode>(4, "Grandchild", ObjectStatus::Broken);
    child1->addChild(grandchild);

    SceneTree originalTree(root);

    // Ensure data directory exists
    std::filesystem::path dataDir("data");
    if (!std::filesystem::exists(dataDir)) {
        std::filesystem::create_directory(dataDir);
    }

    // 2. Save to file
    std::string filename = (dataDir / "test_scene_io.json").string();
    bool saved = SceneIO::saveSceneTree(originalTree, filename);
    ASSERT_TRUE(saved);

    // 3. Load from file
    auto loadedTree = SceneIO::loadSceneTree(filename);
    ASSERT_NE(loadedTree, nullptr);

    // 4. Verify structure
    auto loadedRoot = loadedTree->getRoot();
    ASSERT_NE(loadedRoot, nullptr);
    EXPECT_EQ(loadedRoot->getId(), root->getId());
    EXPECT_EQ(loadedRoot->getName(), root->getName());
    EXPECT_EQ(loadedRoot->getStatus(), root->getStatus());
    
    ASSERT_EQ(loadedRoot->getChildren().size(), 2);
    
    // Verify Child1
    // Note: Order of children is preserved because vector is ordered and serialization/deserialization preserves array order.
    auto loadedChild1 = loadedRoot->getChildren()[0];
    EXPECT_EQ(loadedChild1->getId(), child1->getId());
    EXPECT_EQ(loadedChild1->getName(), child1->getName());
    EXPECT_EQ(loadedChild1->getStatus(), child1->getStatus());

    // Verify Child2
    auto loadedChild2 = loadedRoot->getChildren()[1];
    EXPECT_EQ(loadedChild2->getId(), child2->getId());
    EXPECT_EQ(loadedChild2->getName(), child2->getName());
    EXPECT_EQ(loadedChild2->getStatus(), child2->getStatus());

    // Verify Grandchild
    ASSERT_EQ(loadedChild1->getChildren().size(), 1);
    auto loadedGrandchild = loadedChild1->getChildren()[0];
    EXPECT_EQ(loadedGrandchild->getId(), grandchild->getId());
    EXPECT_EQ(loadedGrandchild->getName(), grandchild->getName());
    EXPECT_EQ(loadedGrandchild->getStatus(), grandchild->getStatus());

    // 5. Cleanup
    std::filesystem::remove(filename);
}

TEST(SceneIOTest, LoadNonExistentFile) {
    auto tree = SceneIO::loadSceneTree("non_existent_file.json");
    EXPECT_EQ(tree, nullptr);
}