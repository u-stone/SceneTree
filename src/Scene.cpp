#include "SceneTree/Scene.h"
#include <algorithm>

Scene::Scene(std::string name) : m_name(std::move(name)) {}

SceneObject* Scene::addObject(ObjectId id, const std::string& name, ObjectStatus status, ObjectId parentId) {
    auto [it, success] = m_objects.try_emplace(id, SceneObject{id, name, status});
    if (success) {
        m_insertion_order.push_back(id);
        m_relationships[id] = parentId;
        return &it->second;
    }
    return nullptr; // Object with this ID already exists
}

SceneObject* Scene::getObject(ObjectId id) {
    auto it = m_objects.find(id);
    if (it != m_objects.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Scene::removeObject(ObjectId id) {
    if (m_objects.erase(id) > 0) {
        m_insertion_order.erase(std::remove(m_insertion_order.begin(), m_insertion_order.end(), id), m_insertion_order.end());
        m_relationships.erase(id);
        return true;
    }
    return false;
}

const std::string& Scene::getName() const {
    return m_name;
}

std::vector<SceneObject*> Scene::getAllObjects() const {
    std::vector<SceneObject*> objects;
    objects.reserve(m_insertion_order.size());
    for (ObjectId id : m_insertion_order) {
        objects.push_back(const_cast<SceneObject*>(&m_objects.at(id)));
    }
    return objects;
}

ObjectId Scene::getParentId(ObjectId id) const {
    auto it = m_relationships.find(id);
    return (it != m_relationships.end()) ? it->second : 0;
}
