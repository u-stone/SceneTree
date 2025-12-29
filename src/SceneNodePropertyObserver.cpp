#include "SceneNodePropertyObserver.h"
#include "SceneTree.h"

void SceneNodePropertyObserver::onNodePropertyChanged(SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal) {
    if (m_tree) {
        m_tree->handlePropertyChange(node, prop, oldVal, newVal);
    }
}