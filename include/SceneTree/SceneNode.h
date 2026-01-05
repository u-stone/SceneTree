#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_set>
#include <functional>
#include <any>
#include <map>
#include <cstdint>
#include "SceneTree/SceneObject.h"

enum class NodeProperty : uint32_t {
    Name       = 1u << 0,
    Status     = 1u << 1,
    TagAdded   = 1u << 2,
    TagRemoved = 1u << 3,
    Visibility = 1u << 4,
    Hierarchy  = 1u << 5,
    IsDirty    = 1u << 6
};

inline NodeProperty operator|(NodeProperty lhs, NodeProperty rhs) {
    return static_cast<NodeProperty>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline NodeProperty operator&(NodeProperty lhs, NodeProperty rhs) {
    return static_cast<NodeProperty>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}

inline NodeProperty& operator|=(NodeProperty& lhs, NodeProperty rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline NodeProperty& operator&=(NodeProperty& lhs, NodeProperty rhs) {
    lhs = lhs & rhs;
    return lhs;
}

class SceneNode;

class INodeObserver {
public:
    virtual ~INodeObserver() = default;
    virtual void onNodePropertyChanged(SceneNode* node, NodeProperty prop, 
                                       const std::any& oldVal, const std::any& newVal) = 0;
};

class SceneNode : public std::enable_shared_from_this<SceneNode> {
public:
    SceneNode(ObjectId id, const std::string& name, ObjectStatus status = ObjectStatus::Active);

    ObjectId getId() const;
    const std::string& getName() const;
    void setName(const std::string& name);
    ObjectStatus getStatus() const;
    void setStatus(ObjectStatus status);

    // Tag management
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    const std::unordered_set<std::string>& getTags() const;

    // Dirty Flag System for Batching Optimization
    bool isPropertyDirty(NodeProperty prop) const;
    bool arePropertiesDirty(NodeProperty mask) const;
    void clearDirty();
    const std::string& getCleanName() const;
    ObjectStatus getCleanStatus() const;

    // Observer mechanism for property changes
    void registerObserver(INodeObserver* observer);
    void unregisterObserver(INodeObserver* observer);

    void addChild(std::shared_ptr<SceneNode> child);
    bool removeChild(const std::shared_ptr<SceneNode>& child);
    const std::vector<std::shared_ptr<SceneNode>>& getChildren() const;

    std::shared_ptr<SceneNode> findFirstChildNodeByName(const std::string& name) const;
    std::vector<std::shared_ptr<SceneNode>> findAllChildNodesByName(const std::string& name) const;

    const std::vector<std::weak_ptr<SceneNode>>& getParents() const;

private:
    friend class SceneTree; // Allow SceneTree to manage parents

    void addParent(std::weak_ptr<SceneNode> parent);
    void removeParent(const std::weak_ptr<SceneNode>& parent);

    void markDirty(NodeProperty prop);

    ObjectId m_id;
    std::string m_name;
    ObjectStatus m_status;
    uint32_t m_dirty_flags = 0;
    std::string m_clean_name;
    ObjectStatus m_clean_status;
    std::unordered_set<std::string> m_tags;
    std::vector<INodeObserver*> m_observers;
    std::vector<std::shared_ptr<SceneNode>> m_children;
    std::vector<std::weak_ptr<SceneNode>> m_parents;
};
