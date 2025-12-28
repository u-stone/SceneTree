# Design Document: SceneTree Management System

## 1. Introduction

This document outlines the design and architecture of the SceneTree management system. The system is designed to provide a flexible and efficient way to manage complex scene hierarchies in a game engine. The core requirement is to support multi-parented scene graphs, where a single scene node or an entire scene tree can be a child of multiple parent scenes simultaneously. This creates a Directed Acyclic Graph (DAG) rather than a simple tree structure.

## 2. Core Components

The system is composed of five main classes:

-   `Scene`: Manages a collection of game objects for a specific scene.
-   `SceneNode`: Represents a node within the scene graph (SceneTree).
-   `SceneTree`: Represents a hierarchy of `SceneNode`s.
-   `SceneManager`: A high-level manager for loading, unloading, and switching between scenes.
-   `SceneIO`: A utility class for serializing and deserializing scene trees to/from JSON.

## 3. Data Structures and Algorithms

### 3.1. `SceneNode`: The Graph Node

A `SceneNode` is the fundamental building block of the scene graph.

-   **ID, Name, Status**: Basic properties for identification and state management. The `id` is the key that links a `SceneNode` to a `SceneObject`.
-   **Parent-Child Relationships**: To implement a DAG, a node must be ableto have multiple parents.
-   **Parent-Child Relationships**: To implement a DAG, a node must be able to have multiple parents.
    -   `m_children`: `std::vector<std::shared_ptr<SceneNode>>`. Children are owned by their parents. `std::shared_ptr` is used because a child node's lifetime is tied to all its parents. It will only be destroyed when the last parent referencing it is destroyed.
    -   `m_parents`: `std::vector<std::weak_ptr<SceneNode>>`. `std::weak_ptr` is crucial here to prevent circular references. If a child held a `shared_ptr` to its parent, and the parent held a `shared_ptr` to the child, a reference cycle would be created, leading to memory leaks. `weak_ptr` allows a node to reference its parents without affecting their lifetime.

### 3.2. `SceneTree`: The Graph

A `SceneTree` encapsulates a scene graph.

-   **Root Node**: A `std::shared_ptr<SceneNode>` acts as the root of the tree/sub-graph.
-   **Fast Node Lookup**: `std::unordered_map<ObjectId, SceneNode*> m_node_lookup;`. This map provides average O(1) time complexity for finding any node in the tree by its unique `ObjectId`. The map stores raw pointers for performance, assuming the `SceneTree` itself manages the lifetime of its nodes through the `m_root`'s ownership of all its children.
-   **Name-based Lookup**: `std::unordered_map<std::string, std::vector<SceneNode*>> m_name_lookup;`.
    -   **Global Lookup**: Provides O(1) access to all nodes with a specific name. Supports duplicate names by storing a vector of pointers.
    -   **Scoped Lookup**: Finds nodes by name within a specific subtree. Instead of traversing the subtree (O(N)), it retrieves all nodes with the target name from the global map and checks if they are descendants of the start node (Ancestry Check).
    -   **Hierarchical Lookup**: Delegates to `SceneNode`'s recursive search for DFS-based lookups (`findFirstChildNodeByName`).

-   **Attach/Detach Algorithm**:
    -   `attach(parentNode, childTree)`:
        1.  The `childTree`'s root node is added to the `parentNode`'s list of children (`m_children`).
        2.  The `parentNode` is added to the `childTree` root's list of parents (`m_parents`).
        3.  The `childTree`'s node lookup map (`m_node_lookup`) and name lookup map (`m_name_lookup`) are merged into the parent `SceneTree`'s maps. A DFS traversal is used to ensure deterministic order for duplicate names.
    -   `detach(parentNode, childNode)`:
        1.  The `childNode` is removed from the `parentNode`'s `m_children` vector.
        2.  The `parentNode` is removed from the `childNode`'s `m_parents` vector.
        3.  The nodes from the detached subtree are removed from the main `SceneTree`'s lookup maps (`m_node_lookup` and `m_name_lookup`).

### 3.3. `Scene`: Object Data Repository

The `Scene` class acts as a data container for the actual game objects.

-   **Object Storage**: `std::unordered_map<ObjectId, SceneObject> m_objects;`. An `unordered_map` is chosen for fast object retrieval by ID, which is essential when a `SceneNode` needs to access its corresponding game object data. This decouples the scene graph's structure (`SceneTree`) from the scene's content (`Scene`). A `SceneNode` refers to a `SceneObject` via its ID.

### 3.4. `SceneManager`: High-Level Control

The `SceneManager` orchestrates the overall scene lifecycle.

-   **Scene Management**: `std::unordered_map<std::string, std::shared_ptr<Scene>> m_scenes;`. Stores all available scenes, mapped by a string name for easy access.
-   **Active Scene Tree**: `std::unique_ptr<SceneTree> m_active_scene_tree;`. The manager owns the main, active scene graph that represents the currently rendered world.
-   **Scene Switching (`switchToScene`)**: This is a high-level operation that would typically involve:
    1.  Creating a new, empty `SceneTree`.
    2.  Loading a target `Scene` from the scene map.
    3.  Building a `SceneTree` from the objects in that `Scene`.
    4.  Potentially attaching other `SceneTree`s (e.g., a persistent UI scene) to the new active tree.
    5.  Replacing the old `m_active_scene_tree` with the new one. This automatically triggers the destruction of the old tree and all its nodes, correctly decrementing `shared_ptr` counts.

### 3.5. `SceneIO`: Serialization

The `SceneIO` class handles the persistence of scene data.

-   **Format**: JSON (via RapidJSON library).
-   **Functionality**:
    -   `saveSceneTree`: Serializes a `SceneTree` structure, including node properties (ID, Name, Status) and hierarchy, to a file.
    -   `loadSceneTree`: Parses a JSON file to reconstruct a `SceneTree` object.

## 4. Design Choices and Justification

-   **DAG vs. Tree**: A traditional tree is too restrictive. A DAG allows for more complex and memory-efficient world-building. For example, a single "lantern" `SceneTree` can be attached to multiple posts in a larger "city" scene without duplicating the lantern's data or structure.
-   **Smart Pointers (`shared_ptr`, `weak_ptr`)**: This is the standard C++ approach for managing complex ownership patterns like a DAG. It automates memory management and prevents common issues like memory leaks from reference cycles and dangling pointers from premature deallocation.
-   **ID-based Lookups (`unordered_map`)**: Hash maps provide the fastest average-case lookup performance, which is critical for real-time applications like games where finding scene nodes for updates, rendering, or physics is a common operation.
-   **Strongly Typed IDs (`ObjectId`)**: Encapsulating the ID in a class (`ObjectIdType<T>`) prevents type mismatch errors (e.g., passing an integer where an ID is expected) and allows changing the underlying ID type (e.g., to `std::string` or UUID) with minimal code changes.
-   **Decoupling of `Scene` and `SceneTree`**: The `Scene` holds the "what" (the data/objects), and the `SceneTree` holds the "where" (the spatial/hierarchical relationships). This separation of concerns makes the system more flexible. For instance, the same `Scene` data could be represented by different `SceneTree` arrangements.
-   **Serialization Strategy**: Using RapidJSON provides a fast and standard way to interchange data. The recursive structure of the JSON directly maps to the recursive structure of the Scene Tree.

## 5. Conclusion

This design provides a robust, flexible, and efficient foundation for scene management. By using a DAG, smart pointers, and hash-based lookups, it meets all the core requirements, including multi-parenting, fast queries, and clean resource management through attach/detach and scene switching operations.
