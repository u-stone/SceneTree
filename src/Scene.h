#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "SceneObject.h"

class Scene {
public:
    explicit Scene(std::string name);

    SceneObject* addObject(unsigned int id, const std::string& name, ObjectStatus status = ObjectStatus::Active, unsigned int parentId = 0);
    SceneObject* getObject(unsigned int id);
    bool removeObject(unsigned int id);

    const std::string& getName() const;
    std::vector<SceneObject*> getAllObjects() const;
    unsigned int getParentId(unsigned int id) const;

private:
    std::string m_name;
    std::unordered_map<unsigned int, SceneObject> m_objects;
    std::vector<unsigned int> m_insertion_order;
    std::unordered_map<unsigned int, unsigned int> m_relationships;
};
