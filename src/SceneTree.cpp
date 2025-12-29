#include "SceneTree.h"
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <iostream>

class SceneNodePropertyObserver : public INodeObserver {
public:
    explicit SceneNodePropertyObserver(SceneTree* tree) : m_tree(tree) {}

    void onNodePropertyChanged(SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal) override {
        if (prop == NodeProperty::IsDirty) {
            m_tree->m_dirty_nodes.push_back(node->weak_from_this());
            if (!m_tree->m_batching_enabled) {
                m_tree->processEvents();
            }
            return;
        }

        if (m_tree->m_batching_enabled) {
            m_tree->m_event_queue.push_back({node->weak_from_this(), prop, oldVal, newVal});
        } else {
            m_tree->handlePropertyChange(node, prop, oldVal, newVal);
        }
    }

private:
    SceneTree* m_tree;
};

SceneTree::SceneTree(std::shared_ptr<SceneNode> root) : m_root(std::move(root)) {
    if (!m_root) {
        throw std::invalid_argument("SceneTree root cannot be null.");
    }
    m_node_observer = std::make_unique<SceneNodePropertyObserver>(this);
    buildNodeMap(m_root);
}
SceneTree::~SceneTree() {
    for (auto const& [id, node_ptr] : m_node_lookup) {
        node_ptr->unregisterObserver(m_node_observer.get());
    }
}

// Static factory function to create a SceneTree from a Scene
std::unique_ptr<SceneTree> SceneTree::createFromScene(const Scene& scene) {
    auto objects = scene.getAllObjects();
    if (objects.empty()) {
        return nullptr;
    }

    std::unordered_map<ObjectId, std::shared_ptr<SceneNode>> node_map;
    std::shared_ptr<SceneNode> root = nullptr;

    // First pass: Create all nodes
    for (auto* obj : objects) {
        node_map[obj->id] = std::make_shared<SceneNode>(obj->id, obj->name, obj->status);
    }

    // Second pass: Build hierarchy
    for (auto* obj : objects) {
        ObjectId parentId = scene.getParentId(obj->id);
        auto currentNode = node_map[obj->id];

        if (parentId == 0 || node_map.find(parentId) == node_map.end()) {
            // If no parent or parent not in this scene, it's a root candidate
            if (!root) root = currentNode; 
        } else {
            node_map[parentId]->addChild(currentNode);
        }
    }

    return root ? std::make_unique<SceneTree>(root) : nullptr;
}


SceneNode* SceneTree::findNode(ObjectId id) {
    auto it = m_node_lookup.find(id);
    if (it != m_node_lookup.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<SceneNode> SceneTree::findNodeByName(const std::string& name) const {
    auto it = m_name_lookup.find(name);
    if (it != m_name_lookup.end() && !it->second.empty()) {
        return it->second.front()->shared_from_this();
    }
    return nullptr;
}

std::shared_ptr<SceneNode> SceneTree::findFirstChildNodeByName(const std::string& name) const {
    if (!m_root) return nullptr;
    if (m_root->getName() == name) return m_root;
    return m_root->findFirstChildNodeByName(name);
}

std::vector<std::shared_ptr<SceneNode>> SceneTree::findAllNodesByName(const std::string& name) const {
    std::vector<std::shared_ptr<SceneNode>> results;
    auto it = m_name_lookup.find(name);
    if (it != m_name_lookup.end()) {
        for (auto* node : it->second) {
            results.push_back(node->shared_from_this());
        }
    }
    return results;
}

std::shared_ptr<SceneNode> SceneTree::findFirstNodeByTag(const std::string& tag) const {
    auto it = m_tag_lookup.find(tag);
    if (it != m_tag_lookup.end() && !it->second.empty()) {
        return it->second.front()->shared_from_this();
    }
    return nullptr;
}

std::vector<std::shared_ptr<SceneNode>> SceneTree::findAllNodesByTag(const std::string& tag) const {
    std::vector<std::shared_ptr<SceneNode>> results;
    auto it = m_tag_lookup.find(tag);
    if (it != m_tag_lookup.end()) {
        results.reserve(it->second.size());
        for (auto* node : it->second) {
            results.push_back(node->shared_from_this());
        }
    }
    return results;
}

static bool isDescendant(SceneNode* node, SceneNode* ancestor) {
    if (!node || !ancestor) return false;

    // Use iterative BFS to avoid recursion depth limits and handle DAGs efficiently
    std::vector<SceneNode*> queue;
    std::unordered_set<SceneNode*> visited;

    queue.push_back(node);
    visited.insert(node);

    size_t head = 0;
    while (head < queue.size()) {
        SceneNode* current = queue[head++];
        
        for (const auto& weak_parent : current->getParents()) {
            if (auto parent = weak_parent.lock()) {
                if (parent.get() == ancestor) return true;
                if (visited.insert(parent.get()).second) {
                    queue.push_back(parent.get());
                }
            }
        }
    }
    
    return false;
}

std::shared_ptr<SceneNode> SceneTree::findNodeByName(SceneNode* startNode, const std::string& name) const {
    if (!startNode) return nullptr;

    // Validate that startNode belongs to this tree
    auto it = m_node_lookup.find(startNode->getId());
    if (it == m_node_lookup.end() || it->second != startNode) {
        return nullptr;
    }

    if (startNode->getName() == name) {
        return startNode->shared_from_this();
    }
    
    auto name_it = m_name_lookup.find(name);
    if (name_it != m_name_lookup.end()) {
        for (auto* node : name_it->second) {
            if (isDescendant(node, startNode)) {
                return node->shared_from_this();
            }
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<SceneNode>> SceneTree::findAllNodesByName(SceneNode* startNode, const std::string& name) const {
    std::vector<std::shared_ptr<SceneNode>> results;
    if (!startNode) return results;

    // Validate that startNode belongs to this tree
    auto it = m_node_lookup.find(startNode->getId());
    if (it == m_node_lookup.end() || it->second != startNode) {
        return results;
    }

    if (startNode->getName() == name) {
        results.push_back(startNode->shared_from_this());
    }
    
    auto name_it = m_name_lookup.find(name);
    if (name_it != m_name_lookup.end()) {
        for (auto* node : name_it->second) {
            if (node != startNode && isDescendant(node, startNode)) {
                results.push_back(node->shared_from_this());
            }
        }
    }
    return results;
}

bool SceneTree::attach(SceneNode* parentNode, std::unique_ptr<SceneTree> childTree) {
    if (!parentNode || !childTree || !childTree->m_root) {
        return false;
    }

    // Ensure the parentNode is actually part of this tree
    if (findNode(parentNode->getId()) != parentNode) {
        return false;
    }

    // Check for ID collisions before modifying the tree
    for (auto const& [id, node_ptr] : childTree->m_node_lookup) {
        auto it = m_node_lookup.find(id);
        if (it != m_node_lookup.end()) {
            if (it->second != node_ptr) {
                return false; // ID collision: same ID but different object instance
            }
        }
    }

    // Transfer ownership and structure
    std::shared_ptr<SceneNode> childRoot = childTree->getRoot();
    parentNode->addChild(childRoot);

    // Merge the node maps using DFS traversal to ensure deterministic order
    buildNodeMap(childRoot);

    // The childTree unique_ptr is now empty, its resources are merged.
    childTree->m_root = nullptr;

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

    // DAG Handling:
    // In a multi-parent scenario, nodes in the detached subtree might still be 
    // reachable from the main tree root via other parents. We must NOT remove 
    // those nodes from the lookup maps.
    
    std::unordered_set<SceneNode*> retainedNodes;

    // 1. Identify "Entry Points": Nodes in the detached set that have parents 
    //    still existing in the main tree (and not part of the detached set).
    for (auto const& [id, node_ptr] : detachedTree->m_node_lookup) {
        for (const auto& weakP : node_ptr->getParents()) {
            if (auto p = weakP.lock()) {
                // If parent is in main tree BUT NOT in detached tree
                if (m_node_lookup.count(p->getId()) && 
                    detachedTree->m_node_lookup.find(p->getId()) == detachedTree->m_node_lookup.end()) {
                    retainedNodes.insert(node_ptr);
                }
            }
        }
    }

    // 2. Propagate retention: If a node is retained, all its descendants must be retained.
    std::vector<SceneNode*> queue(retainedNodes.begin(), retainedNodes.end());
    size_t head = 0;
    while(head < queue.size()){
        SceneNode* curr = queue[head++];
        for(auto& child : curr->getChildren()){
            // If child is in detached tree (it must be) and not yet retained
            if (detachedTree->m_node_lookup.count(child->getId())) {
                if (retainedNodes.find(child.get()) == retainedNodes.end()) {
                    retainedNodes.insert(child.get());
                    queue.push_back(child.get());
                }
            }
        }
    }

    // 3. Erase only nodes that are NOT retained
    for (auto const& [id, node_ptr] : detachedTree->m_node_lookup) {
        if (retainedNodes.find(node_ptr) == retainedNodes.end()) {
            m_node_lookup.erase(id);
            
            auto it = m_name_lookup.find(node_ptr->getName());
            if (it != m_name_lookup.end()) {
                auto& vec = it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), node_ptr), vec.end());
                if (vec.empty()) {
                    m_name_lookup.erase(it);
                }
            }

            // Remove from tag lookup
            for (const auto& tag : node_ptr->getTags()) {
                auto tag_it = m_tag_lookup.find(tag);
                if (tag_it != m_tag_lookup.end()) {
                    auto& vec = tag_it->second;
                    vec.erase(std::remove(vec.begin(), vec.end(), node_ptr), vec.end());
                    if (vec.empty()) {
                        m_tag_lookup.erase(tag_it);
                    }
                }
            }

            // Stop observing
            node_ptr->unregisterObserver(m_node_observer.get());
        }
    }
    
    return detachedTree;
}

std::shared_ptr<SceneNode> SceneTree::getRoot() const {
    return m_root;
}

void SceneTree::print() const {
    if (!m_root) return;

    auto print_recursive = [](const SceneNode& node, int depth, auto&& self) -> void {
        std::string indent(depth * 4, ' ');
        std::cout << indent << "- " << node.getName() << " (ID: " << node.getId() 
                  << ", Status: " << node.getStatus() 
                  << ", Parents: " << node.getParents().size() << ")" << std::endl;
        for (const auto& child : node.getChildren()) {
            self(*child, depth + 1, self);
        }
    };

    print_recursive(*m_root, 0, print_recursive);
}

void SceneTree::addPropertyListener(NodeProperty prop, PropertyListener listener) {
    m_global_listeners[prop].push_back(std::move(listener));
}

void SceneTree::addNodePropertyListener(ObjectId id, NodeProperty prop, PropertyListener listener) {
    m_node_listeners[prop][id].push_back(std::move(listener));
}

void SceneTree::setBatchingEnabled(bool enabled) {
    m_batching_enabled = enabled;
    if (!enabled) {
        processEvents();
    }
}

void SceneTree::update(double deltaTime) {
    // Currently, we just process pending property changes.
    // In the future, this is where we would update animations, spatial partitions, etc.
    processEvents();
}

void SceneTree::processEvents() {
    // 1. Process Dirty Nodes (Batched Updates)
    if (!m_dirty_nodes.empty()) {
        std::vector<std::weak_ptr<SceneNode>> nodes;
        nodes.swap(m_dirty_nodes);

        for (auto& weak_node : nodes) {
            if (auto node = weak_node.lock()) {
                if (m_node_lookup.count(node->getId())) {
                    resolveDirtyNode(node.get());
                }
            }
        }
    }

    // 2. Process Event Queue (Immediate/Non-batched events like Tags)
    if (m_event_queue.empty()) return;

    // Swap queue to allow new events to be added safely during processing
    std::vector<PendingEvent> processing_queue;
    processing_queue.swap(m_event_queue);

    for (const auto& event : processing_queue) {
        if (auto node_ptr = event.node.lock()) {
            // Ensure the node is still part of this tree before processing
            if (m_node_lookup.find(node_ptr->getId()) != m_node_lookup.end()) {
                handlePropertyChange(node_ptr.get(), event.prop, event.oldVal, event.newVal);
            }
        }
    }
}

void SceneTree::resolveDirtyNode(SceneNode* node) {
    // Optimization: Check if any relevant properties are dirty using the bitmask.
    // This avoids individual checks if the node is dirty due to unhandled properties.
    if (!node->arePropertiesDirty(NodeProperty::Name | NodeProperty::Status)) {
        node->clearDirty();
        return;
    }

    // Handle Name Changes
    if (node->arePropertiesDirty(NodeProperty::Name)) {
        std::string oldName = node->getCleanName();
        std::string newName = node->getName();
        
        // Update Index
        auto it = m_name_lookup.find(oldName);
        if (it != m_name_lookup.end()) {
            auto& vec = it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), node), vec.end());
            if (vec.empty()) m_name_lookup.erase(it);
        }
        m_name_lookup[newName].push_back(node);

        // Notify Listeners
        if (auto it = m_global_listeners.find(NodeProperty::Name); it != m_global_listeners.end()) {
            for (auto& listener : it->second) listener(node, NodeProperty::Name, oldName, newName);
        }
        if (auto it = m_node_listeners.find(NodeProperty::Name); it != m_node_listeners.end()) {
            if (auto node_it = it->second.find(node->getId()); node_it != it->second.end()) {
                for (auto& listener : node_it->second) listener(node, NodeProperty::Name, oldName, newName);
            }
        }
    }

    // Handle Status Changes
    if (node->arePropertiesDirty(NodeProperty::Status)) {
        ObjectStatus oldStatus = node->getCleanStatus();
        ObjectStatus newStatus = node->getStatus();

        // Notify Listeners
        if (auto it = m_global_listeners.find(NodeProperty::Status); it != m_global_listeners.end()) {
            for (auto& listener : it->second) listener(node, NodeProperty::Status, oldStatus, newStatus);
        }
        if (auto it = m_node_listeners.find(NodeProperty::Status); it != m_node_listeners.end()) {
            if (auto node_it = it->second.find(node->getId()); node_it != it->second.end()) {
                for (auto& listener : node_it->second) listener(node, NodeProperty::Status, oldStatus, newStatus);
            }
        }
    }

    node->clearDirty();
}

void SceneTree::handlePropertyChange(SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal) {
    // 1. Update internal indices
    switch (prop) {
        // Name is now handled via Dirty Flag system in resolveDirtyNode
        case NodeProperty::TagAdded: {
            std::string tag = std::any_cast<std::string>(newVal);
            m_tag_lookup[tag].push_back(node);
            break;
        }
        case NodeProperty::TagRemoved: {
            std::string tag = std::any_cast<std::string>(oldVal);
            auto it = m_tag_lookup.find(tag);
            if (it != m_tag_lookup.end()) {
                auto& vec = it->second;
                vec.erase(std::remove(vec.begin(), vec.end(), node), vec.end());
                if (vec.empty()) m_tag_lookup.erase(it);
            }
            break;
        }
        default: break;
    }

    // 2. Notify global listeners
    if (auto it = m_global_listeners.find(prop); it != m_global_listeners.end()) {
        for (auto& listener : it->second) listener(node, prop, oldVal, newVal);
    }

    // 3. Notify node-specific listeners
    if (auto it = m_node_listeners.find(prop); it != m_node_listeners.end()) {
        if (auto node_it = it->second.find(node->getId()); node_it != it->second.end()) {
            for (auto& listener : node_it->second) listener(node, prop, oldVal, newVal);
        }
    }
}

void SceneTree::buildNodeMap(const std::shared_ptr<SceneNode>& node) {
    if (!node) return;
    m_node_lookup[node->getId()] = node.get();
    m_name_lookup[node->getName()].push_back(node.get());

    for (const auto& tag : node->getTags()) m_tag_lookup[tag].push_back(node.get());
    
    node->registerObserver(m_node_observer.get());

    for (const auto& child : node->getChildren()) {
        buildNodeMap(child);
    }
}

void SceneTree::removeNodeMap(const std::shared_ptr<SceneNode>& node) {
    if (!node) return;
    m_node_lookup.erase(node->getId());
    
    auto it = m_name_lookup.find(node->getName());
    if (it != m_name_lookup.end()) {
        auto& vec = it->second;
        vec.erase(std::remove(vec.begin(), vec.end(), node.get()), vec.end());
        if (vec.empty()) {
            m_name_lookup.erase(it);
        }
    }

    // Remove tags from index
    for (const auto& tag : node->getTags()) {
        auto tag_it = m_tag_lookup.find(tag);
        if (tag_it != m_tag_lookup.end()) {
            auto& vec = tag_it->second;
            vec.erase(std::remove(vec.begin(), vec.end(), node.get()), vec.end());
            if (vec.empty()) {
                m_tag_lookup.erase(tag_it);
            }
        }
    }

    node->unregisterObserver(m_node_observer.get());

    for (const auto& child : node->getChildren()) {
        removeNodeMap(child);
    }
}
