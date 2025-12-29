#include "SceneNode.h"
#include "SceneObject.h"
#include <algorithm>
#include <stdexcept>

// Helper to check if 'potentialAncestor' is actually an ancestor of 'node'
static bool isNodeAncestor(const SceneNode* potentialAncestor, const SceneNode* node) {
    if (potentialAncestor == node) return true;
    for (const auto& weakParent : node->getParents()) {
        if (auto parent = weakParent.lock()) {
            if (isNodeAncestor(potentialAncestor, parent.get())) return true;
        }
    }
    return false;
}

SceneNode::SceneNode(ObjectId id, const std::string& name, ObjectStatus status)
    : m_id(id), m_name(name), m_status(status) {}

ObjectId SceneNode::getId() const {
    return m_id;
}

const std::string& SceneNode::getName() const {
    return m_name;
}

void SceneNode::setName(const std::string& name) {
    if (m_name == name) return;
    if (!isPropertyDirty(NodeProperty::Name)) {
        m_clean_name = m_name;
        m_name = name;
        markDirty(NodeProperty::Name);
    } else {
        m_name = name;
    }
}

ObjectStatus SceneNode::getStatus() const {
    return m_status;
}

void SceneNode::setStatus(ObjectStatus status) {
    if (m_status == status) return;
    if (!isPropertyDirty(NodeProperty::Status)) {
        m_clean_status = m_status;
        m_status = status;
        markDirty(NodeProperty::Status);
    } else {
        m_status = status;
    }
}

void SceneNode::markDirty(NodeProperty prop) {
    bool was_clean = (m_dirty_flags == 0);
    m_dirty_flags |= static_cast<uint32_t>(prop);
    
    // Only notify observers when transitioning from Clean to Dirty.
    // Subsequent updates are coalesced until clearDirty() is called.
    if (was_clean) {
        for (auto* observer : m_observers) {
            observer->onNodePropertyChanged(this, NodeProperty::IsDirty, std::any(), std::any());
        }
    }
}

bool SceneNode::isPropertyDirty(NodeProperty prop) const {
    return (m_dirty_flags & static_cast<uint32_t>(prop)) != 0;
}

bool SceneNode::arePropertiesDirty(NodeProperty mask) const {
    return (m_dirty_flags & static_cast<uint32_t>(mask)) != 0;
}

void SceneNode::clearDirty() {
    m_dirty_flags = 0;
}

const std::string& SceneNode::getCleanName() const {
    return m_clean_name;
}

ObjectStatus SceneNode::getCleanStatus() const {
    return m_clean_status;
}

void SceneNode::addTag(const std::string& tag) {
    if (m_tags.insert(tag).second) {
        for (auto* observer : m_observers) {
            observer->onNodePropertyChanged(this, NodeProperty::TagAdded, std::any(), tag);
        }
    }
}

void SceneNode::removeTag(const std::string& tag) {
    if (m_tags.erase(tag) > 0) {
        for (auto* observer : m_observers) {
            observer->onNodePropertyChanged(this, NodeProperty::TagRemoved, tag, std::any());
        }
    }
}

bool SceneNode::hasTag(const std::string& tag) const {
    return m_tags.find(tag) != m_tags.end();
}

const std::unordered_set<std::string>& SceneNode::getTags() const {
    return m_tags;
}

void SceneNode::registerObserver(INodeObserver* observer) {
    if (observer && std::find(m_observers.begin(), m_observers.end(), observer) == m_observers.end()) {
        m_observers.push_back(observer);
    }
}

void SceneNode::unregisterObserver(INodeObserver* observer) {
    m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer), m_observers.end());
}

void SceneNode::addChild(std::shared_ptr<SceneNode> child) {
    if (!child) return;
    
    // Check for self-parenting
    if (child.get() == this) {
        throw std::invalid_argument("A node cannot be its own child.");
    }

    // Check for cycles: ensure 'this' is not an ancestor of 'child'
    if (isNodeAncestor(child.get(), this)) {
        throw std::runtime_error("Cycle detected: Cannot add an ancestor as a child.");
    }
    
    m_children.push_back(child);
    child->addParent(weak_from_this());
}

bool SceneNode::removeChild(const std::shared_ptr<SceneNode>& child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->removeParent(weak_from_this());
        m_children.erase(it);
        return true;
    }
    return false;
}

const std::vector<std::shared_ptr<SceneNode>>& SceneNode::getChildren() const {
    return m_children;
}

const std::vector<std::weak_ptr<SceneNode>>& SceneNode::getParents() const {
    return m_parents;
}

void SceneNode::addParent(std::weak_ptr<SceneNode> parent) {
    m_parents.push_back(parent);
}

void SceneNode::removeParent(const std::weak_ptr<SceneNode>& parent) {
    m_parents.erase(std::remove_if(m_parents.begin(), m_parents.end(),
        [&parent](const std::weak_ptr<SceneNode>& p) {
            return !parent.owner_before(p) && !p.owner_before(parent);
        }), m_parents.end());
}

std::shared_ptr<SceneNode> SceneNode::findFirstChildNodeByName(const std::string& name) const {
    for (const auto& child : m_children) {
        if (child) {
            if (child->getName() == name) {
                return child;
            }
            if (auto found = child->findFirstChildNodeByName(name)) {
                return found;
            }
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<SceneNode>> SceneNode::findAllChildNodesByName(const std::string& name) const {
    std::vector<std::shared_ptr<SceneNode>> results;
    for (const auto& child : m_children) {
        if (child) {
            if (child->getName() == name) {
                results.push_back(child);
            }
            auto child_results = child->findAllChildNodesByName(name);
            results.insert(results.end(), child_results.begin(), child_results.end());
        }
    }
    return results;
}
