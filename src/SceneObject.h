#pragma once

#include <string>
#include <ostream>
#include <functional>
#include <atomic>
#include <type_traits>

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

inline std::string statusToString(ObjectStatus status) {
    switch (status) {
        case ObjectStatus::Active:   return "Active";
        case ObjectStatus::Inactive: return "Inactive";
        case ObjectStatus::Hidden:   return "Hidden";
        case ObjectStatus::Broken:   return "Broken";
        default:                     return "Unknown";
    }
}

inline ObjectStatus statusFromString(const std::string& str) {
    if (str == "Inactive") return ObjectStatus::Inactive;
    if (str == "Hidden")   return ObjectStatus::Hidden;
    if (str == "Broken")   return ObjectStatus::Broken;
    return ObjectStatus::Active;
}

template <typename T>
class ObjectIdType {
public:
    ObjectIdType() : m_id(T()) {}
    ObjectIdType(const T& id) : m_id(id) {}

    bool operator==(const ObjectIdType& other) const { return m_id == other.m_id; }
    bool operator!=(const ObjectIdType& other) const { return m_id != other.m_id; }
    
    T raw() const { return m_id; }

    // Internal algorithm: Generate unique ID
    static ObjectIdType generate() {
        static std::atomic<size_t> counter{1000000}; // Start from a large number to avoid conflicts with manually assigned low IDs
        size_t val = ++counter;
        if constexpr (std::is_same_v<T, std::string>) {
            return ObjectIdType(std::to_string(val));
        } else {
            return ObjectIdType(static_cast<T>(val));
        }
    }

private:
    T m_id;
};

namespace std {
    template <typename T> struct hash<ObjectIdType<T>> {
        size_t operator()(const ObjectIdType<T>& id) const {
            return hash<T>{}(id.raw());
        }
    };
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const ObjectIdType<T>& id) {
    return os << id.raw();
}

using ObjectId = ObjectIdType<unsigned int>;

// A simple structure representing an object in a scene.
struct SceneObject {
    ObjectId id;
    std::string name;
    ObjectStatus status;
};
