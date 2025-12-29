#pragma once
#include "SceneNode.h"
#include <any>
#include "SceneTree.h"

class SceneTree;

class SceneNodePropertyObserver : public INodeObserver {
public:
    SceneNodePropertyObserver(SceneTree* tree) : m_tree(tree) {}
    void onNodePropertyChanged(SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal) override;
private:
    SceneTree* m_tree;