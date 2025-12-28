#include "gtest/gtest.h"
#include "SceneManager.h"
#include "Scene.h"
#include "SceneNode.h"
#include "SceneTree.h"

// Test fixture for SceneManager tests
class SceneManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<SceneManager>();

        // Create and register a main scene
        auto main_scene = std::make_shared<Scene>("MainScene");
        main_scene->addObject(1, "Root");
        main_scene->addObject(2, "Player", ObjectStatus::Active, 1);
        main_scene->addObject(3, "Enemy", ObjectStatus::Active, 1);
        manager->registerScene(main_scene);

        // Create and register a sub-scene
        auto sub_scene = std::make_shared<Scene>("SubScene");
        sub_scene->addObject(10, "SubRoot");
        sub_scene->addObject(11, "Item", ObjectStatus::Active, 10);
        manager->registerScene(sub_scene);
    }

    std::unique_ptr<SceneManager> manager;
};

TEST_F(SceneManagerTest, SceneRegistration) {
    EXPECT_NE(manager->getScene("MainScene"), nullptr);
    EXPECT_NE(manager->getScene("SubScene"), nullptr);
    EXPECT_EQ(manager->getScene("InvalidScene"), nullptr);
}

TEST_F(SceneManagerTest, SwitchScene) {
    EXPECT_EQ(manager->getActiveSceneTree(), nullptr);
    
    bool success = manager->switchToScene("MainScene");
    EXPECT_TRUE(success);
    
    SceneTree* activeTree = manager->getActiveSceneTree();
    ASSERT_NE(activeTree, nullptr);
    EXPECT_EQ(activeTree->getRoot()->getId(), 1); // Root ID of MainScene
    EXPECT_NE(activeTree->findNode(2), nullptr); // Player
}

TEST_F(SceneManagerTest, AttachScene) {
    manager->switchToScene("MainScene");
    
    bool attached = manager->attachScene("MainScene", "SubScene", 2); // Attach sub-scene to player node
    EXPECT_TRUE(attached);

    SceneTree* activeTree = manager->getActiveSceneTree();
    ASSERT_NE(activeTree, nullptr);

    SceneNode* playerNode = activeTree->findNode(2);
    ASSERT_NE(playerNode, nullptr);
    EXPECT_EQ(playerNode->getChildren().size(), 1);

    SceneNode* subSceneRoot = activeTree->findNode(10);
    EXPECT_NE(subSceneRoot, nullptr);
    EXPECT_EQ(playerNode->getChildren()[0], subSceneRoot->shared_from_this());

    // Check parentage
    EXPECT_EQ(subSceneRoot->getParents().size(), 1);
    auto parent = subSceneRoot->getParents()[0].lock();
    EXPECT_EQ(parent.get(), playerNode);
}
