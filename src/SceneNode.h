#pragma once

#include <string>
#include <vector>
#include <memory>
#include "SceneObject.h"

class SceneNode : public std::enable_shared_from_this<SceneNode> {
public:
    SceneNode(unsigned int id, const std::string& name, const std::string& status = "active");

    unsigned int getId() const;
    const std::string& getName() const;
    const std::string& getStatus() const;
    void setStatus(const std::string& status);

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

    unsigned int m_id;
    std::string m_name;
    std::string m_status;
    std::vector<std::shared_ptr<SceneNode>> m_children;
    std::vector<std::weak_ptr<SceneNode>> m_parents;
};
