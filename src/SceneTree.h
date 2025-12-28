#pragma once

#include <memory>
#include <unordered_map>
#include "SceneNode.h"
#include "Scene.h"

class SceneTree {
public:
    explicit SceneTree(std::shared_ptr<SceneNode> root);

    static std::unique_ptr<SceneTree> createFromScene(const Scene& scene);

    SceneNode* findNode(unsigned int id);
    
    // Attaches another tree to a specific node in this tree
    bool attach(SceneNode* parentNode, std::unique_ptr<SceneTree> childTree);

    // Detaches a subtree starting at childNode from its parent parentNode
    std::unique_ptr<SceneTree> detach(SceneNode* parentNode, SceneNode* childNode);

    std::shared_ptr<SceneNode> getRoot() const;

private:
    void buildNodeMap(const std::shared_ptr<SceneNode>& node);
    void removeNodeMap(const std::shared_ptr<SceneNode>& node);


    std::shared_ptr<SceneNode> m_root;
    std::unordered_map<unsigned int, SceneNode*> m_node_lookup;
};
