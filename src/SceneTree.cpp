#include "SceneTree.h"
#include <stdexcept>
#include <vector>

SceneTree::SceneTree(std::shared_ptr<SceneNode> root) : m_root(std::move(root)) {
    if (!m_root) {
        throw std::invalid_argument("SceneTree root cannot be null.");
    }
    buildNodeMap(m_root);
}

// Static factory function to create a SceneTree from a Scene
std::unique_ptr<SceneTree> SceneTree::createFromScene(const Scene& scene) {
    auto objects = scene.getAllObjects();
    if (objects.empty()) {
        return nullptr;
    }

    // Assume the first object is the root, a common convention.
    // A more robust implementation might look for an object with no parent pointers.
    auto root_obj = objects[0];
    auto root_node = std::make_shared<SceneNode>(root_obj->id, root_obj->name, root_obj->status);
    
    auto tree = std::make_unique<SceneTree>(root_node);

    // This is a simplified creation. A real implementation would need to know the hierarchy
    // from the scene data itself. Here we just add all other objects as direct children of the root.
    for (size_t i = 1; i < objects.size(); ++i) {
        auto obj = objects[i];
        auto node = std::make_shared<SceneNode>(obj->id, obj->name, obj->status);
        tree->m_root->addChild(node);
        tree->m_node_lookup[node->getId()] = node.get();
    }

    return tree;
}


SceneNode* SceneTree::findNode(unsigned int id) {
    auto it = m_node_lookup.find(id);
    if (it != m_node_lookup.end()) {
        return it->second;
    }
    return nullptr;
}

bool SceneTree::attach(SceneNode* parentNode, std::unique_ptr<SceneTree> childTree) {
    if (!parentNode || !childTree || !childTree->m_root) {
        return false;
    }

    // Ensure the parentNode is actually part of this tree
    if (findNode(parentNode->getId()) != parentNode) {
        return false;
    }

    // Transfer ownership and structure
    std::shared_ptr<SceneNode> childRoot = childTree->getRoot();
    parentNode->addChild(childRoot);

    // Merge the node maps
    for (auto const& [id, node_ptr] : childTree->m_node_lookup) {
        m_node_lookup[id] = node_ptr;
    }

    // The childTree unique_ptr is now empty, its resources are merged.
    childTree->m_root = nullptr;
    childTree->m_node_lookup.clear();

    return true;
}

std::unique_ptr<SceneTree> SceneTree::detach(SceneNode* parentNode, SceneNode* childNode) {
    if (!parentNode || !childNode) {
        return nullptr;
    }
    
    auto childSharedPtr = childNode->shared_from_this();

    if (!parentNode->removeChild(childSharedPtr)) {
        // The childNode is not a direct child of parentNode.
        return nullptr;
    }

    // Create a new SceneTree for the detached part.
    // The SceneTree constructor will rebuild the node map for the detached subtree.
    auto detachedTree = std::make_unique<SceneTree>(childSharedPtr);

    // Remove the detached nodes from the current tree's lookup map.
    for (auto const& [id, node_ptr] : detachedTree->m_node_lookup) {
        m_node_lookup.erase(id);
    }
    
    return detachedTree;
}

std::shared_ptr<SceneNode> SceneTree::getRoot() const {
    return m_root;
}

void SceneTree::buildNodeMap(const std::shared_ptr<SceneNode>& node) {
    if (!node) return;
    m_node_lookup[node->getId()] = node.get();
    for (const auto& child : node->getChildren()) {
        buildNodeMap(child);
    }
}

void SceneTree::removeNodeMap(const std::shared_ptr<SceneNode>& node) {
    if (!node) return;
    m_node_lookup.erase(node->getId());
    for (const auto& child : node->getChildren()) {
        removeNodeMap(child);
    }
}
