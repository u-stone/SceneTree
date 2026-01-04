#pragma once
#include "SceneNode.h"
#include <any>

class SceneTree;

class SceneNodePropertyObserver : public INodeObserver {
public:
    explicit SceneNodePropertyObserver(SceneTree* tree) : m_tree(tree) {}
    void onNodePropertyChanged(SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal) override;
private:
    SceneTree* m_tree;
};