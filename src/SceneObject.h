#pragma once

#include <string>
#include <ostream>

enum class ObjectStatus {
    Active,
    Inactive,
    Hidden,
    Broken
};

inline std::ostream& operator<<(std::ostream& os, ObjectStatus status) {
    switch (status) {
        case ObjectStatus::Active:   return os << "Active";
        case ObjectStatus::Inactive: return os << "Inactive";
        case ObjectStatus::Hidden:   return os << "Hidden";
        case ObjectStatus::Broken:   return os << "Broken";
        default:                     return os << "Unknown";
    }
}

// A simple structure representing an object in a scene.
struct SceneObject {
    unsigned int id;
    std::string name;
    ObjectStatus status;
};
