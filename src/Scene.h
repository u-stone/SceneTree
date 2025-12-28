#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "SceneObject.h"

class Scene {
public:
    explicit Scene(std::string name);

    SceneObject* addObject(unsigned int id, const std::string& name, const std::string& status = "active");
    SceneObject* getObject(unsigned int id);
    bool removeObject(unsigned int id);

    const std::string& getName() const;
    std::vector<SceneObject*> getAllObjects() const;

private:
    std::string m_name;
    std::unordered_map<unsigned int, SceneObject> m_objects;
};
