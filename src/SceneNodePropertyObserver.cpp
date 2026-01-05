#include "SceneTree/SceneNodePropertyObserver.h"
#include "SceneTree.h"

void SceneNodePropertyObserver::onNodePropertyChanged(SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal) {
    if (!m_tree) return;

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