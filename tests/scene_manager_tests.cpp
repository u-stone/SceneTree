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
        main_scene->addObject(2, "Player");
        main_scene->addObject(3, "Enemy");
        manager->registerScene(main_scene);

        // Create and register a sub-scene
        auto sub_scene = std::make_shared<Scene>("SubScene");
        sub_scene->addObject(10, "SubRoot");
        sub_scene->addObject(11, "Item");
        manager->registerScene(sub_scene);
    }

    std::unique_ptr<SceneManager> manager;
};

TEST(SceneNodeTest, Creation) {
    auto node = std::make_shared<SceneNode>(1, "TestNode", "active");
    EXPECT_EQ(node->getId(), 1);
    EXPECT_EQ(node->getName(), "TestNode");
    EXPECT_EQ(node->getStatus(), "active");
    EXPECT_TRUE(node->getChildren().empty());
    EXPECT_TRUE(node->getParents().empty());
}

TEST(SceneNodeTest, AddAndRemoveChild) {
    auto parent = std::make_shared<SceneNode>(1, "Parent");
    auto child = std::make_shared<SceneNode>(2, "Child");

    parent->addChild(child);
    ASSERT_EQ(parent->getChildren().size(), 1);
    EXPECT_EQ(parent->getChildren()[0], child);
    ASSERT_EQ(child->getParents().size(), 1);

    // Check if weak_ptr points to parent
    auto parent_from_child = child->getParents()[0].lock();
    EXPECT_EQ(parent_from_child, parent);

    parent->removeChild(child);
    EXPECT_TRUE(parent->getChildren().empty());
    EXPECT_TRUE(child->getParents().empty());
}

TEST(SceneTreeTest, FindNode) {
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto child = std::make_shared<SceneNode>(2, "Child");
    root->addChild(child);

    auto tree = std::make_unique<SceneTree>(root);
    
    SceneNode* foundRoot = tree->findNode(1);
    SceneNode* foundChild = tree->findNode(2);
    SceneNode* notFound = tree->findNode(99);

    EXPECT_EQ(foundRoot, root.get());
    EXPECT_EQ(foundChild, child.get());
    EXPECT_EQ(notFound, nullptr);
}

TEST(SceneTreeTest, AttachAndDetach) {
    // Tree A
    auto rootA = std::make_shared<SceneNode>(1, "RootA");
    auto treeA = std::make_unique<SceneTree>(rootA);

    // Tree B
    auto rootB = std::make_shared<SceneNode>(10, "RootB");
    auto childB = std::make_shared<SceneNode>(11, "ChildB");
    rootB->addChild(childB);
    auto treeB = std::make_unique<SceneTree>(rootB);

    // Attach B to A
    treeA->attach(rootA.get(), std::move(treeB));
    
    ASSERT_EQ(rootA->getChildren().size(), 1);
    EXPECT_EQ(rootA->getChildren()[0], rootB);
    EXPECT_NE(treeA->findNode(10), nullptr);
    EXPECT_NE(treeA->findNode(11), nullptr);

    // Detach B from A
    auto detachedTree = treeA->detach(rootA.get(), rootB.get());
    
    EXPECT_TRUE(rootA->getChildren().empty());
    EXPECT_EQ(treeA->findNode(10), nullptr);
    ASSERT_NE(detachedTree, nullptr);
    EXPECT_EQ(detachedTree->getRoot(), rootB);
    EXPECT_NE(detachedTree->findNode(10), nullptr);
    EXPECT_NE(detachedTree->findNode(11), nullptr);
}

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

TEST(SceneNodeTest, MultiParent) {
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto nodeA = std::make_shared<SceneNode>(2, "NodeA");
    auto sharedChild = std::make_shared<SceneNode>(10, "SharedChild");

    root->addChild(nodeA);
    root->addChild(sharedChild); // root is the first parent
    nodeA->addChild(sharedChild); // nodeA is the second parent

    ASSERT_EQ(sharedChild->getParents().size(), 2);
    
    // Remove from one parent
    root->removeChild(sharedChild);
    ASSERT_EQ(sharedChild->getParents().size(), 1);
    auto onlyParent = sharedChild->getParents()[0].lock();
    EXPECT_EQ(onlyParent, nodeA);
}
