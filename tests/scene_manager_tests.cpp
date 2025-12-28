#include "gtest/gtest.h"
#include <algorithm>
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

TEST(SceneNodeTest, DetectCycle) {
    auto nodeA = std::make_shared<SceneNode>(1, "A");
    auto nodeB = std::make_shared<SceneNode>(2, "B");
    auto nodeC = std::make_shared<SceneNode>(3, "C");

    nodeA->addChild(nodeB);
    nodeB->addChild(nodeC);

    // Try to add A as child of C (A->B->C->A)
    EXPECT_THROW(nodeC->addChild(nodeA), std::runtime_error);
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

TEST(SceneNodeTest, FindByName) {
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto child1 = std::make_shared<SceneNode>(2, "Camera");
    auto child2 = std::make_shared<SceneNode>(3, "Player");
    auto child3 = std::make_shared<SceneNode>(4, "Enemy");
    auto grandchild1 = std::make_shared<SceneNode>(5, "Weapon");
    auto grandchild2 = std::make_shared<SceneNode>(6, "Armor");
    auto grandchild3 = std::make_shared<SceneNode>(7, "Weapon");

    root->addChild(child1);
    root->addChild(child2);
    root->addChild(child3);
    child2->addChild(grandchild1);
    child2->addChild(grandchild2);
    child3->addChild(grandchild3);

    // Test findFirstChildNodeByName
    auto first_weapon = root->findFirstChildNodeByName("Weapon");
    ASSERT_NE(first_weapon, nullptr);
    EXPECT_EQ(first_weapon, grandchild1);
    
    auto player_weapon = child2->findFirstChildNodeByName("Weapon");
    ASSERT_NE(player_weapon, nullptr);
    EXPECT_EQ(player_weapon, grandchild1);

    auto no_node = root->findFirstChildNodeByName("NonExistent");
    EXPECT_EQ(no_node, nullptr);

    // Test findAllChildNodesByName
    auto all_weapons = root->findAllChildNodesByName("Weapon");
    ASSERT_EQ(all_weapons.size(), 2);
    // The order should be deterministic based on DFS traversal.
    EXPECT_EQ(all_weapons[0], grandchild1);
    EXPECT_EQ(all_weapons[1], grandchild3);

    auto all_players = root->findAllChildNodesByName("Player");
    ASSERT_EQ(all_players.size(), 1);
    EXPECT_EQ(all_players[0], child2);

    auto no_nodes = root->findAllChildNodesByName("NonExistent");
    EXPECT_TRUE(no_nodes.empty());
}

TEST(SceneTreeTest, FindNodeByName) {
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto child1 = std::make_shared<SceneNode>(2, "Target");
    auto child2 = std::make_shared<SceneNode>(3, "Target"); // Duplicate name
    
    root->addChild(child1);
    root->addChild(child2);
    
    auto tree = std::make_unique<SceneTree>(root);
    
    // Test findNodeByName (should find first occurrence or root)
    auto foundRoot = tree->findNodeByName("Root");
    ASSERT_NE(foundRoot, nullptr);
    EXPECT_EQ(foundRoot, root);

    auto foundTarget = tree->findNodeByName("Target");
    ASSERT_NE(foundTarget, nullptr);
    EXPECT_EQ(foundTarget->getName(), "Target");

    // Test findAllNodesByName
    auto allTargets = tree->findAllNodesByName("Target");
    EXPECT_EQ(allTargets.size(), 2);

    auto allRoots = tree->findAllNodesByName("Root");
    EXPECT_EQ(allRoots.size(), 1);
}

TEST(SceneTreeTest, FindNodeByNameScoped) {
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto branch = std::make_shared<SceneNode>(2, "Branch");
    auto leaf = std::make_shared<SceneNode>(3, "Leaf");
    
    root->addChild(branch);
    branch->addChild(leaf);
    
    auto tree = std::make_unique<SceneTree>(root);
    
    // Find branch first to get the start node
    SceneNode* branchPtr = tree->findNode(2);
    ASSERT_NE(branchPtr, nullptr);
    
    // Search for Leaf starting from Branch
    auto foundLeaf = tree->findNodeByName(branchPtr, "Leaf");
    ASSERT_NE(foundLeaf, nullptr);
    EXPECT_EQ(foundLeaf->getId(), 3);
    
    // Search for Root starting from Branch (should fail as Root is parent, not child)
    auto foundRoot = tree->findNodeByName(branchPtr, "Root");
    EXPECT_EQ(foundRoot, nullptr);
}

TEST(SceneTreeTest, AttachDetachNamingCollision) {
    // 1. Setup Tree A with a node named "CommonName"
    auto rootA = std::make_shared<SceneNode>(1, "RootA");
    auto nodeA = std::make_shared<SceneNode>(2, "CommonName");
    rootA->addChild(nodeA);
    auto treeA = std::make_unique<SceneTree>(rootA);

    // 2. Setup Tree B with a node named "CommonName" (Same name, different ID/Instance)
    auto rootB = std::make_shared<SceneNode>(10, "RootB");
    auto nodeB = std::make_shared<SceneNode>(11, "CommonName");
    rootB->addChild(nodeB);
    auto treeB = std::make_unique<SceneTree>(rootB);

    // 3. Attach B to A
    treeA->attach(rootA.get(), std::move(treeB));

    // 4. Verify lookup finds BOTH "CommonName" nodes
    auto foundNodes = treeA->findAllNodesByName("CommonName");
    EXPECT_EQ(foundNodes.size(), 2);
    
    // Verify we have both ID 2 and ID 11
    bool hasId2 = std::any_of(foundNodes.begin(), foundNodes.end(), [](auto& n){ return n->getId() == 2; });
    bool hasId11 = std::any_of(foundNodes.begin(), foundNodes.end(), [](auto& n){ return n->getId() == 11; });
    EXPECT_TRUE(hasId2);
    EXPECT_TRUE(hasId11);

    // 5. Detach B from A (using rootB's ID 10 to find the split point)
    auto rootBPtr = treeA->findNode(10);
    ASSERT_NE(rootBPtr, nullptr);
    
    auto detachedTree = treeA->detach(rootA.get(), rootBPtr);

    // 6. Verify Tree A still has its own "CommonName" node (ID 2), but ID 11 is gone
    auto foundNodesAfter = treeA->findAllNodesByName("CommonName");
    ASSERT_EQ(foundNodesAfter.size(), 1);
    EXPECT_EQ(foundNodesAfter[0]->getId(), 2);
    EXPECT_EQ(foundNodesAfter[0]->getName(), "CommonName");
}

TEST(SceneTreeTest, DetachSharedNodeDAG) {
    // Scenario: Root -> A -> B
    //           Root -> C -> B
    // B is shared. If we detach A from Root, B should still be accessible via C.

    auto root = std::make_shared<SceneNode>(1, "Root");
    auto nodeA = std::make_shared<SceneNode>(2, "A");
    auto nodeC = std::make_shared<SceneNode>(3, "C");
    auto nodeB = std::make_shared<SceneNode>(4, "B");

    root->addChild(nodeA);
    root->addChild(nodeC);
    nodeA->addChild(nodeB);
    nodeC->addChild(nodeB);

    auto tree = std::make_unique<SceneTree>(root);

    // Verify initial state
    EXPECT_NE(tree->findNode(4), nullptr); // B is found

    // Detach A from Root
    auto detachedTree = tree->detach(root.get(), nodeA.get());

    // A should be gone from main tree
    EXPECT_EQ(tree->findNode(2), nullptr);

    // B should STILL be in main tree because it's a child of C
    EXPECT_NE(tree->findNode(4), nullptr);

    // Verify detached tree contains A and B
    EXPECT_NE(detachedTree->findNode(2), nullptr);
    EXPECT_NE(detachedTree->findNode(4), nullptr);
}

TEST(SceneTreeTest, AttachIDCollision) {
    // Tree A: Root(1) -> Child(2)
    auto rootA = std::make_shared<SceneNode>(1, "RootA");
    auto childA = std::make_shared<SceneNode>(2, "Child");
    rootA->addChild(childA);
    auto treeA = std::make_unique<SceneTree>(rootA);

    // Tree B: Root(10) -> Child(2) [Different object, same ID]
    auto rootB = std::make_shared<SceneNode>(10, "RootB");
    auto childB = std::make_shared<SceneNode>(2, "ChildCollision");
    rootB->addChild(childB);
    auto treeB = std::make_unique<SceneTree>(rootB);

    // Attempt attach
    bool result = treeA->attach(rootA.get(), std::move(treeB));
    
    // Should fail due to ID 2 collision
    EXPECT_FALSE(result);
    
    // Verify treeA state is unchanged
    EXPECT_EQ(rootA->getChildren().size(), 1);
    EXPECT_EQ(rootA->getChildren()[0], childA);
}

TEST(SceneTreeTest, DAG_Diamond_AttachTwice) {
    // Construct a Diamond: Root -> A -> Shared
    //                       Root -> B -> Shared
    
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto nodeA = std::make_shared<SceneNode>(2, "A");
    auto nodeB = std::make_shared<SceneNode>(3, "B");
    auto shared = std::make_shared<SceneNode>(4, "Shared");

    root->addChild(nodeA);
    root->addChild(nodeB);

    auto tree = std::make_unique<SceneTree>(root);

    // 1. Attach Shared to A
    // We need to wrap Shared in a SceneTree to use attach
    auto treeWrapper1 = std::make_unique<SceneTree>(shared);
    EXPECT_TRUE(tree->attach(nodeA.get(), std::move(treeWrapper1)));

    // Verify A -> Shared
    EXPECT_EQ(nodeA->getChildren().size(), 1);
    EXPECT_EQ(nodeA->getChildren()[0], shared);
    EXPECT_EQ(shared->getParents().size(), 1);

    // 2. Attach Shared to B
    // Wrap the SAME shared node in a new SceneTree wrapper
    auto treeWrapper2 = std::make_unique<SceneTree>(shared);
    EXPECT_TRUE(tree->attach(nodeB.get(), std::move(treeWrapper2)));

    // Verify B -> Shared
    EXPECT_EQ(nodeB->getChildren().size(), 1);
    EXPECT_EQ(nodeB->getChildren()[0], shared);
    
    // Verify Shared has 2 parents
    EXPECT_EQ(shared->getParents().size(), 2);

    // Verify Lookup
    EXPECT_EQ(tree->findNode(4), shared.get());

    // 3. Detach from A
    auto detached1 = tree->detach(nodeA.get(), shared.get());
    
    // Shared should still be in the tree (reachable via B)
    EXPECT_NE(tree->findNode(4), nullptr);
    EXPECT_EQ(shared->getParents().size(), 1); // Only B left
    
    // 4. Detach from B
    auto detached2 = tree->detach(nodeB.get(), shared.get());

    // Shared should now be gone from the tree
    EXPECT_EQ(tree->findNode(4), nullptr);
    EXPECT_EQ(shared->getParents().size(), 0);
}

TEST(SceneTreeTest, SharedNode_Across_Two_Trees) {
    // Tree 1: Root1 -> Shared
    // Tree 2: Root2 -> Shared
    // Modifying Shared in Tree 1 should reflect in Tree 2

    auto root1 = std::make_shared<SceneNode>(1, "Root1");
    auto root2 = std::make_shared<SceneNode>(2, "Root2");
    auto shared = std::make_shared<SceneNode>(3, "Shared", "active");

    auto tree1 = std::make_unique<SceneTree>(root1);
    auto tree2 = std::make_unique<SceneTree>(root2);

    // Attach to Tree 1
    auto wrapper1 = std::make_unique<SceneTree>(shared);
    tree1->attach(root1.get(), std::move(wrapper1));

    // Attach to Tree 2
    auto wrapper2 = std::make_unique<SceneTree>(shared);
    tree2->attach(root2.get(), std::move(wrapper2));

    // Verify existence
    EXPECT_NE(tree1->findNode(3), nullptr);
    EXPECT_NE(tree2->findNode(3), nullptr);

    // Modify via Tree 1
    SceneNode* nodeInTree1 = tree1->findNode(3);
    nodeInTree1->setStatus("inactive");

    // Check via Tree 2
    SceneNode* nodeInTree2 = tree2->findNode(3);
    EXPECT_EQ(nodeInTree2->getStatus(), "inactive");
    EXPECT_EQ(nodeInTree1, nodeInTree2); // Should be same pointer
}

TEST(SceneTreeTest, AttachCycleDetection) {
    // Root -> A
    // Try to attach Root to A (Cycle)
    
    auto root = std::make_shared<SceneNode>(1, "Root");
    auto nodeA = std::make_shared<SceneNode>(2, "A");
    root->addChild(nodeA);
    
    auto tree = std::make_unique<SceneTree>(root);
    
    // Wrap root in a new tree to try and attach it to A
    auto wrapper = std::make_unique<SceneTree>(root);
    
    // attach(parentNode, childTree) -> nodeA->addChild(root) -> Should throw Cycle Detected
    EXPECT_THROW(tree->attach(nodeA.get(), std::move(wrapper)), std::runtime_error);
}
