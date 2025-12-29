#include "gtest/gtest.h"
#include "SceneNode.h"
#include <stdexcept>

TEST(SceneNodeTest, Creation) {
    auto node = std::make_shared<SceneNode>(1, "TestNode", ObjectStatus::Active);
    EXPECT_EQ(node->getId(), 1);
    EXPECT_EQ(node->getName(), "TestNode");
    EXPECT_EQ(node->getStatus(), ObjectStatus::Active);
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

TEST(SceneNodeTest, TagManagement) {
    auto node = std::make_shared<SceneNode>(1, "Node");

    node->addTag("Tag1");
    EXPECT_TRUE(node->hasTag("Tag1"));
    
    node->addTag("Tag2");
    EXPECT_EQ(node->getTags().size(), 2);

    node->removeTag("Tag1");
    EXPECT_FALSE(node->hasTag("Tag1"));
    EXPECT_TRUE(node->hasTag("Tag2"));
}

TEST(SceneNodeTest, DirtyFlagSystem) {
    auto node = std::make_shared<SceneNode>(1, "Node");

    // Initially clean
    EXPECT_FALSE(node->isPropertyDirty(NodeProperty::Name));

    // Change name
    node->setName("Changed");
    
    // Should be dirty now
    EXPECT_TRUE(node->isPropertyDirty(NodeProperty::Name));
    EXPECT_EQ(node->getCleanName(), "Node"); // Original name preserved
    EXPECT_EQ(node->getName(), "Changed");

    // Clear dirty
    node->clearDirty();
    EXPECT_FALSE(node->isPropertyDirty(NodeProperty::Name));
    
    // Change again
    node->setName("ChangedAgain");
    EXPECT_EQ(node->getCleanName(), "Changed"); // Previous committed name
}

TEST(SceneNodeTest, DirtyFlagBitwiseOperations) {
    auto node = std::make_shared<SceneNode>(1, "Node");

    // Initially clean
    EXPECT_FALSE(node->arePropertiesDirty(NodeProperty::Name | NodeProperty::Status));

    // Change name -> Dirty Name
    node->setName("NewName");
    EXPECT_TRUE(node->isPropertyDirty(NodeProperty::Name));
    EXPECT_FALSE(node->isPropertyDirty(NodeProperty::Status));
    
    // Check combined mask: (Name | Status) & DirtyFlags(Name) -> Non-zero
    EXPECT_TRUE(node->arePropertiesDirty(NodeProperty::Name | NodeProperty::Status));
    
    // Change status -> Dirty Status
    node->setStatus(ObjectStatus::Inactive);
    EXPECT_TRUE(node->isPropertyDirty(NodeProperty::Status));
    
    // Test bitwise operators helper
    NodeProperty mask = NodeProperty::Name;
    mask |= NodeProperty::Status;
    EXPECT_EQ(mask, NodeProperty::Name | NodeProperty::Status);
    
    mask &= NodeProperty::Name;
    EXPECT_EQ(mask, NodeProperty::Name);
}