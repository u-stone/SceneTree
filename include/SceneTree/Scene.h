#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "SceneObject.h"

class Scene {
public:
    explicit Scene(std::string name);

    SceneObject* addObject(ObjectId id, const std::string& name, ObjectStatus status = ObjectStatus::Active, ObjectId parentId = 0);
    SceneObject* getObject(ObjectId id);
    bool removeObject(ObjectId id);

    const std::string& getName() const;
    std::vector<SceneObject*> getAllObjects() const;
    ObjectId getParentId(ObjectId id) const;

private:
    std::string m_name;
    std::unordered_map<ObjectId, SceneObject> m_objects;
    std::vector<ObjectId> m_insertion_order;
    std::unordered_map<ObjectId, ObjectId> m_relationships;
};
