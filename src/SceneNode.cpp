#include "SceneNode.h"
#include <algorithm>
#include <stdexcept>

SceneNode::SceneNode(unsigned int id, const std::string& name, const std::string& status)
    : m_id(id), m_name(name), m_status(status) {}

unsigned int SceneNode::getId() const {
    return m_id;
}

const std::string& SceneNode::getName() const {
    return m_name;
}

const std::string& SceneNode::getStatus() const {
    return m_status;
}

void SceneNode::setStatus(const std::string& status) {
    m_status = status;
}

void SceneNode::addChild(std::shared_ptr<SceneNode> child) {
    if (!child) return;
    
    // Check for self-parenting
    if (child.get() == this) {
        throw std::invalid_argument("A node cannot be its own child.");
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
