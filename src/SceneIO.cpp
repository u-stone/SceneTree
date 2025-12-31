#include "SceneIO.h"
#include "SceneObject.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static const int CURRENT_FORMAT_VERSION = 1;

// Helper function to serialize a single node recursively
static void serializeNode(json& j_node, const std::shared_ptr<SceneNode>& node) {
    if (!node) return;

    j_node["id"] = node->getId().raw();
    j_node["name"] = node->getName();
    j_node["status"] = statusToString(node->getStatus());

    // --- Tags ---
    const auto& tags = node->getTags();
    if (!tags.empty()) {
        j_node["tags"] = tags;
    }

    // --- Future Extensions ---

    // --- Children (Recursive) ---
    const auto& children = node->getChildren();
    if (!children.empty()) {
        json j_children = json::array();
        for (const auto& child : children) {
            json j_child;
            serializeNode(j_child, child);
            j_children.push_back(j_child);
        }
        j_node["children"] = j_children;
    }
}

bool SceneIO::saveSceneTree(const SceneTree& tree, const std::string& filepath) {
    auto root = tree.getRoot();
    if (!root) {
        std::cerr << "[SceneIO] Error: SceneTree has no root." << std::endl;
        return false;
    }

    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        std::cerr << "[SceneIO] Error: Could not open file for writing: " << filepath << std::endl;
        return false;
    }

    json j;
    j["format_version"] = CURRENT_FORMAT_VERSION;
    
    json j_root;
    serializeNode(j_root, root);
    j["root"] = j_root;

    ofs << j.dump(4);
    return true;
}

// Helper function to deserialize a single node recursively
static std::shared_ptr<SceneNode> deserializeNode(const json& val, int version) {
    if (!val.is_object()) return nullptr;

    // --- Core Properties ---
    if (!val.contains("id") || !val["id"].is_number_unsigned()) {
        std::cerr << "[SceneIO] Warning: Node missing valid 'id'." << std::endl;
        return nullptr;
    }
    ObjectId id(val["id"].get<unsigned int>());

    std::string name = "Unnamed";
    if (val.contains("name") && val["name"].is_string()) {
        name = val["name"].get<std::string>();
    }

    ObjectStatus status = ObjectStatus::Active;
    if (val.contains("status") && val["status"].is_string()) {
        status = statusFromString(val["status"].get<std::string>());
    }

    // Create the node
    auto node = std::make_shared<SceneNode>(id, name, status);

    // --- Tags ---
    if (val.contains("tags") && val["tags"].is_array()) {
        for (const auto& tag : val["tags"]) {
            if (tag.is_string()) {
                node->addTag(tag.get<std::string>());
            }
        }
    }

    // --- Future Extensions ---
    // Parse additional properties here

    // --- Children (Recursive) ---
    if (val.contains("children") && val["children"].is_array()) {
        for (const auto& childVal : val["children"]) {
            auto childNode = deserializeNode(childVal, version);
            if (childNode) {
                node->addChild(childNode);
            }
        }
    }

    return node;
}

std::unique_ptr<SceneTree> SceneIO::loadSceneTree(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        std::cerr << "[SceneIO] Error: Could not open file for reading: " << filepath << std::endl;
        return nullptr;
    }

    json doc;
    try {
        ifs >> doc;
    } catch (const json::parse_error& e) {
        std::cerr << "[SceneIO] JSON Parse Error: " << e.what() << std::endl;
        return nullptr;
    }

    if (!doc.is_object()) {
        return nullptr;
    }

    std::shared_ptr<SceneNode> rootNode = nullptr;
    int version = 0;

    // Check for versioning
    if (doc.contains("format_version") && doc["format_version"].is_number_integer()) {
        version = doc["format_version"].get<int>();
        
        if (version > CURRENT_FORMAT_VERSION) {
            std::cerr << "[SceneIO] Warning: File version (" << version << ") is newer than supported version (" << CURRENT_FORMAT_VERSION << ")." << std::endl;
        }

        if (doc.contains("root")) {
            rootNode = deserializeNode(doc["root"], version);
        }
    } else {
        // Legacy format: The document root is the SceneNode
        rootNode = deserializeNode(doc, 0);
    }

    if (!rootNode) {
        return nullptr;
    }

    return std::make_unique<SceneTree>(rootNode);
}