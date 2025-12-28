#include "Scene.h"
#include <algorithm>

Scene::Scene(std::string name) : m_name(std::move(name)) {}

SceneObject* Scene::addObject(unsigned int id, const std::string& name, const std::string& status) {
    auto [it, success] = m_objects.try_emplace(id, SceneObject{id, name, status});
    if (success) {
        return &it->second;
    }
    return nullptr; // Object with this ID already exists
}

SceneObject* Scene::getObject(unsigned int id) {
    auto it = m_objects.find(id);
    if (it != m_objects.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Scene::removeObject(unsigned int id) {
    return m_objects.erase(id) > 0;
}

const std::string& Scene::getName() const {
    return m_name;
}

std::vector<SceneObject*> Scene::getAllObjects() const {
    std::vector<SceneObject*> objects;
    objects.reserve(m_objects.size());
    for (const auto& pair : m_objects) {
        // C++17 doesn't allow direct conversion from const value_type to non-const value_type
        // so we need to find the object again to get a non-const pointer.
        objects.push_back(const_cast<SceneObject*>(&pair.second));
    }
    return objects;
}
