# Design Document: SceneTree Management System

## 1. Introduction

This document outlines the design and architecture of the SceneTree management system. The system provides a flexible and high-performance way to manage complex scene hierarchies. Beyond supporting Directed Acyclic Graphs (DAG) via multi-parenting, the system incorporates performance optimizations like batched property updates, asynchronous resource loading, and a decoupled observer architecture.

## 2. Core Components

The system is composed of the following core components:

-   `Scene`: Manages a collection of game objects for a specific scene.
-   `SceneNode`: Represents a node within the scene graph (SceneTree).
-   `SceneTree`: Represents a hierarchy of `SceneNode`s.
-   `SceneManager`: A high-level manager for loading, unloading, and switching between scenes.
-   `SceneIO`: A utility class for serializing and deserializing scene trees to/from JSON.
-   `AsyncOperation`: A handle for polling and managing the state of asynchronous tasks.
-   `SceneNodePropertyObserver`: An intermediate layer that decouples property change logic from the tree structure.

## 3. Data Structures and Algorithms

### 3.1. `SceneNode`: The Graph Node

A `SceneNode` is the fundamental building block of the scene graph.

-   **ID, Name, Status**: Basic properties for identification and state management. The `id` is the key that links a `SceneNode` to a `SceneObject`.
-   **Tags**: A collection of strings (`std::unordered_set<std::string>`) used for categorizing nodes for fast retrieval by systems (e.g., AI, Physics, Scripts).
-   **Parent-Child Relationships**: To implement a DAG, a node must be ableto have multiple parents.
-   **Parent-Child Relationships**: To implement a DAG, a node must be able to have multiple parents.
    -   `m_children`: `std::vector<std::shared_ptr<SceneNode>>`. Children are owned by their parents. `std::shared_ptr` is used because a child node's lifetime is tied to all its parents. It will only be destroyed when the last parent referencing it is destroyed.
    -   `m_parents`: `std::vector<std::weak_ptr<SceneNode>>`. `std::weak_ptr` is crucial here to prevent circular references. If a child held a `shared_ptr` to its parent, and the parent held a `shared_ptr` to the child, a reference cycle would be created, leading to memory leaks. `weak_ptr` allows a node to reference its parents without affecting their lifetime.

### 3.2. `SceneTree`: The Graph

A `SceneTree` encapsulates a scene graph.

-   **Root Node**: A `std::shared_ptr<SceneNode>` acts as the root of the tree/sub-graph.
-   **Fast Node Lookup**: `std::unordered_map<ObjectId, SceneNode*> m_node_lookup;`. This map provides average O(1) time complexity for finding any node in the tree by its unique `ObjectId`. The map stores raw pointers for performance, assuming the `SceneTree` itself manages the lifetime of its nodes through the `m_root`'s ownership of all its children.
-   **Batching System**: When enabled, property changes (like name or status) are queued. The `update(deltaTime)` method processes these "dirty" nodes in a single pass, minimizing the overhead of updating internal lookup maps.
-   **Name-based Lookup**: `std::unordered_map<std::string, std::vector<SceneNode*>> m_name_lookup;`.
    -   **Global Lookup**: Provides O(1) access to all nodes with a specific name. Supports duplicate names by storing a vector of pointers.
    -   **Scoped Lookup**: Finds nodes by name within a specific subtree. It retrieves candidates from the global map and performs an optimized **Ancestry Check** using an iterative BFS with a visited set to handle deep trees and DAGs efficiently.
    -   **Hierarchical Lookup**: Delegates to `SceneNode`'s recursive search for DFS-based lookups (`findFirstChildNodeByName`).

-   **Tag-based Lookup**: `std::unordered_map<std::string, std::vector<SceneNode*>> m_tag_lookup;`.
    -   Provides O(1) access to groups of nodes categorized by functional tags (e.g., "Enemy", "Interactable", "Checkpoint").
    -   Essential for script systems to efficiently query sets of objects without traversing the hierarchy or relying on unique names.

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
-   **Asynchronous Loading**: Supports `loadSceneAsync` and `preloadSceneAsync`. These methods offload I/O and tree construction to background threads, returning a `shared_ptr<AsyncOperation>` for polling.
-   **Task Merging**: If multiple requests are made for the same scene simultaneously, the manager merges them into a single loading task, notifying all callers upon completion.
-   **Asynchronous Unloading**: `unloadSceneAsync` moves the destruction of large scene trees to a background thread, preventing frame-rate spikes on the main thread.
-   **Update Loop**: The `update()` method must be called per frame to harvest completed async tasks and trigger callbacks on the main thread.

### 3.5. `AsyncOperation`: Polling Handle

The `AsyncOperation` class provides a thread-safe way to track the progress of asynchronous tasks.

-   **IsDone()**: Non-blocking check for completion.
-   **GetResult()**: Retrieves the success/failure result, blocking only if the task is still in progress.
-   **Callbacks**: Supports optional `SceneAsyncCallback` for event-driven completion handling.

### 3.5. `SceneIO`: Serialization

The `SceneIO` class handles the persistence of scene data.

-   **Format**: JSON (via RapidJSON library).
-   **Functionality**:
    -   `saveSceneTree`: Serializes a `SceneTree` structure, including node properties (ID, Name, Status, Tags) and hierarchy.
    -   `loadSceneTree`: Parses a JSON file to reconstruct a `SceneTree` object.
-   **Versioning**: Includes a `format_version` field to ensure forward and backward compatibility as the scene schema evolves.

## 4. Design Choices and Justification

-   **DAG vs. Tree**: A traditional tree is too restrictive. A DAG allows for more complex and memory-efficient world-building. For example, a single "lantern" `SceneTree` can be attached to multiple posts in a larger "city" scene without duplicating the lantern's data or structure.
-   **Smart Pointers (`shared_ptr`, `weak_ptr`)**: This is the standard C++ approach for managing complex ownership patterns like a DAG. It automates memory management and prevents common issues like memory leaks from reference cycles and dangling pointers from premature deallocation.
-   **ID-based Lookups (`unordered_map`)**: Hash maps provide the fastest average-case lookup performance, which is critical for real-time applications like games where finding scene nodes for updates, rendering, or physics is a common operation.
-   **Strongly Typed IDs (`ObjectId`)**: Encapsulating the ID in a class (`ObjectIdType<T>`) prevents type mismatch errors (e.g., passing an integer where an ID is expected) and allows changing the underlying ID type (e.g., to `std::string` or UUID) with minimal code changes.
-   **Decoupling of `Scene` and `SceneTree`**: The `Scene` holds the "what" (the data/objects), and the `SceneTree` holds the "where" (the spatial/hierarchical relationships). This separation of concerns makes the system more flexible. For instance, the same `Scene` data could be represented by different `SceneTree` arrangements.
-   **Serialization Strategy**: Using RapidJSON provides a fast and standard way to interchange data. The recursive structure of the JSON directly maps to the recursive structure of the Scene Tree.
-   **Batching and Update Loop**: By deferring index updates to a single point in the frame, we trade immediate consistency for significantly higher throughput, which is essential for dynamic scenes.
-   **Async/Polling Hybrid**: Providing both callbacks and polling handles (`AsyncOperation`) gives developers the flexibility to use the pattern that best fits their specific use case (e.g., UI vs. State Machines).
-   **Observer Decoupling**: Using `SceneNodePropertyObserver` prevents `SceneTree` from becoming a "God Object" and allows for specialized handling strategies based on node types.

## 5. Conclusion

This design provides a robust, flexible, and high-performance foundation for scene management. By combining DAG structures, smart pointers, batched updates, and asynchronous I/O, it meets the demands of modern game engines for both complexity and efficiency.
