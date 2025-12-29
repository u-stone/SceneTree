#include "SceneIO.h"
#include "SceneObject.h"
#include <fstream>
#include <iostream>
#include <vector>

// RapidJSON headers
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h" // For human-readable JSON

using namespace rapidjson;

static const int CURRENT_FORMAT_VERSION = 1;

// Helper function to serialize a single node recursively
static void serializeNode(PrettyWriter<OStreamWrapper>& writer, const std::shared_ptr<SceneNode>& node) {
    if (!node) return;

    writer.StartObject();

    // --- Core Properties ---
    writer.Key("id");
    writer.Uint(node->getId().raw());

    writer.Key("name");
    writer.String(node->getName().c_str());

    writer.Key("status");
    writer.String(statusToString(node->getStatus()).c_str());

    // --- Tags ---
    const auto& tags = node->getTags();
    if (!tags.empty()) {
        writer.Key("tags");
        writer.StartArray();
        for (const auto& tag : tags) {
            writer.String(tag.c_str());
        }
        writer.EndArray();
    }

    // --- Future Extensions ---
    // You can add more properties here, e.g.:
    // writer.Key("transform"); writer.StartObject(); ... writer.EndObject();

    // --- Children (Recursive) ---
    const auto& children = node->getChildren();
    if (!children.empty()) {
        writer.Key("children");
        writer.StartArray();
        for (const auto& child : children) {
            serializeNode(writer, child);
        }
        writer.EndArray();
    }

    writer.EndObject();
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

    OStreamWrapper osw(ofs);
    PrettyWriter<OStreamWrapper> writer(osw);

    // Wrap the output in a versioned object
    writer.StartObject();

    writer.Key("format_version");
    writer.Int(CURRENT_FORMAT_VERSION);

    writer.Key("root");
    serializeNode(writer, root);
    writer.EndObject();

    return true;
}

// Helper function to deserialize a single node recursively
static std::shared_ptr<SceneNode> deserializeNode(const Value& val, int version) {
    if (!val.IsObject()) return nullptr;

    // --- Core Properties ---
    if (!val.HasMember("id") || !val["id"].IsUint()) {
        std::cerr << "[SceneIO] Warning: Node missing valid 'id'." << std::endl;
        return nullptr;
    }
    ObjectId id(val["id"].GetUint());

    std::string name = "Unnamed";
    if (val.HasMember("name") && val["name"].IsString()) {
        name = val["name"].GetString();
    }

    ObjectStatus status = ObjectStatus::Active;
    if (val.HasMember("status") && val["status"].IsString()) {
        status = statusFromString(val["status"].GetString());
    }

    // Create the node
    auto node = std::make_shared<SceneNode>(id, name, status);

    // --- Tags ---
    if (val.HasMember("tags") && val["tags"].IsArray()) {
        const Value& tags = val["tags"];
        for (SizeType i = 0; i < tags.Size(); i++) {
            if (tags[i].IsString()) {
                node->addTag(tags[i].GetString());
            }
        }
    }

    // --- Future Extensions ---
    // Parse additional properties here

    // --- Children (Recursive) ---
    if (val.HasMember("children") && val["children"].IsArray()) {
        const Value& children = val["children"];
        for (SizeType i = 0; i < children.Size(); i++) {
            auto childNode = deserializeNode(children[i], version);
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

    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError()) {
        std::cerr << "[SceneIO] JSON Parse Error: " << doc.GetParseError() << std::endl;
        return nullptr;
    }

    if (!doc.IsObject()) {
        return nullptr;
    }

    std::shared_ptr<SceneNode> rootNode = nullptr;
    int version = 0;

    // Check for versioning
    if (doc.HasMember("format_version") && doc["format_version"].IsInt()) {
        version = doc["format_version"].GetInt();
        
        if (version > CURRENT_FORMAT_VERSION) {
            std::cerr << "[SceneIO] Warning: File version (" << version << ") is newer than supported version (" << CURRENT_FORMAT_VERSION << ")." << std::endl;
        }

        if (doc.HasMember("root")) {
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