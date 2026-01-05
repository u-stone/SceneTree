#pragma once

#include <memory>
#include <unordered_map>
#include "SceneTree/SceneNode.h"
#include "SceneTree/Scene.h"
#include <any>
#include <functional>

class SceneTree {
private:
public:
    explicit SceneTree(std::shared_ptr<SceneNode> root);
    virtual ~SceneTree();

    static std::unique_ptr<SceneTree> createFromScene(const Scene& scene);

    SceneNode* findNode(ObjectId id);
    
    // Find nodes by name (delegates to SceneNode's recursive search)
    std::shared_ptr<SceneNode> findNodeByName(const std::string& name) const;
    std::shared_ptr<SceneNode> findFirstChildNodeByName(const std::string& name) const;
    std::vector<std::shared_ptr<SceneNode>> findAllNodesByName(const std::string& name) const;

    // Tag-based lookup for script systems and fast retrieval
    std::shared_ptr<SceneNode> findFirstNodeByTag(const std::string& tag) const;
    std::vector<std::shared_ptr<SceneNode>> findAllNodesByTag(const std::string& tag) const;

    // Overloads to start finding from a specific node
    std::shared_ptr<SceneNode> findNodeByName(SceneNode* startNode, const std::string& name) const;
    std::vector<std::shared_ptr<SceneNode>> findAllNodesByName(SceneNode* startNode, const std::string& name) const;

    // Attaches another tree to a specific node in this tree
    bool attach(SceneNode* parentNode, std::unique_ptr<SceneTree> childTree);

    // Detaches a subtree starting at childNode from its parent parentNode
    std::unique_ptr<SceneTree> detach(SceneNode* parentNode, SceneNode* childNode);

    void setBatchingEnabled(bool enabled);
    void processEvents();

    // Standard update method to be called once per frame by the SceneManager or GameLoop
    void update(double deltaTime);

    // Centralized property listener registration
    using PropertyListener = std::function<void(SceneNode*, NodeProperty, const std::any&, const std::any&)>;
    void addPropertyListener(NodeProperty prop, PropertyListener listener);
    void addNodePropertyListener(ObjectId id, NodeProperty prop, PropertyListener listener);

    std::shared_ptr<SceneNode> getRoot() const;

    void print() const;

private:
    void buildNodeMap(const std::shared_ptr<SceneNode>& node);
    void removeNodeMap(const std::shared_ptr<SceneNode>& node);
    void resolveDirtyNode(SceneNode* node);
    void handlePropertyChange(SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal);
    friend class SceneNodePropertyObserver;

    struct PendingEvent {
        std::weak_ptr<SceneNode> node;
        NodeProperty prop;
        std::any oldVal;
        std::any newVal;
    };
    std::vector<std::weak_ptr<SceneNode>> m_dirty_nodes;
    std::vector<PendingEvent> m_event_queue;
    bool m_batching_enabled = false;


    std::unique_ptr<INodeObserver> m_node_observer;
    std::shared_ptr<SceneNode> m_root;
    std::unordered_map<ObjectId, SceneNode*> m_node_lookup;
    std::unordered_map<std::string, std::vector<SceneNode*>> m_name_lookup;
    std::unordered_map<std::string, std::vector<SceneNode*>> m_tag_lookup;
    std::unordered_map<NodeProperty, std::vector<PropertyListener>> m_global_listeners;
    std::unordered_map<NodeProperty, std::unordered_map<ObjectId, std::vector<PropertyListener>>> m_node_listeners;
};
