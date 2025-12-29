#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include "SceneManager.h"
#include "SceneIO.h"

int main() {
    try {
        // 1. Setup SceneManager and Scenes
        auto manager = std::make_unique<SceneManager>();

        // Create a "World" scene
        auto world_scene = std::make_shared<Scene>("World");
        world_scene->addObject(1, "WorldRoot");
        world_scene->addObject(10, "Player", ObjectStatus::Active, 1);
        world_scene->addObject(20, "Environment", ObjectStatus::Active, 1);
        world_scene->addObject(21, "Ground", ObjectStatus::Active, 20);
        world_scene->addObject(22, "Sky", ObjectStatus::Active, 20);
        world_scene->addObject(23, "Lamp", ObjectStatus::Active, 20); // Add a duplicate name to demonstrate lookup features

        // Create a "Props" scene that can be attached
        auto props_scene = std::make_shared<Scene>("Props");
        props_scene->addObject(100, "PropsRoot");
        props_scene->addObject(101, "Lamp", ObjectStatus::Active, 100);
        props_scene->addObject(102, "Bench", ObjectStatus::Active, 100);
        
        // Register scenes with the manager
        manager->registerScene(world_scene);
        manager->registerScene(props_scene);

        // 2. Switch to the main world scene
        std::cout << "---- Switching to World Scene ----" << std::endl;
        manager->switchToScene("World");

        SceneTree* active_tree = manager->getActiveSceneTree();
        if (active_tree) {
            std::cout << "Active Scene Tree:" << std::endl;
            active_tree->print();
        }

        // 3. Attach the props scene to the environment node
        std::cout << "\n---- Attaching Props Scene to Environment (ID: 20) ----" << std::endl;
        bool attached = manager->attachScene("World", "Props", 20);

        if (attached && active_tree) {
            std::cout << "Attach successful. Updated Scene Tree:" << std::endl;
            active_tree->print();

            // 4. Find a node and update its status
            std::cout << "\n---- Finding Lamp (ID: 101) and setting status to 'Broken' ----" << std::endl;
            SceneNode* lamp_node = active_tree->findNode(101);
            if (lamp_node) {
                lamp_node->setStatus(ObjectStatus::Broken);
                std::cout << "Lamp status updated. Final Tree:" << std::endl;
                active_tree->print();
            } else {
                std::cerr << "Could not find lamp node!" << std::endl;
            }

            // 4.1. Demonstrate Name Lookup (New Feature)
            std::cout << "\n---- Finding all nodes named 'Lamp' (Global Lookup) ----" << std::endl;
            auto lamps = active_tree->findAllNodesByName("Lamp");
            std::cout << "Found " << lamps.size() << " nodes named 'Lamp':" << std::endl;
            for (const auto& node : lamps) {
                std::cout << "  - ID: " << node->getId() << ", Status: " << node->getStatus() << std::endl;
            }

            // 4.2. Demonstrate Scoped Lookup (New Feature)
            std::cout << "\n---- Finding 'Lamp' scoped under 'Environment' (ID: 20) ----" << std::endl;
            auto env_node = active_tree->findNode(20);
            if (env_node) {
                auto scoped_lamp = active_tree->findNodeByName(env_node, "Lamp");
                if (scoped_lamp) {
                    std::cout << "Found scoped Lamp: ID " << scoped_lamp->getId() << " (Expected ID 101 from Props scene)" << std::endl;
                }
            }

            // 4.3. Demonstrate Hierarchical Lookup (New Feature)
            std::cout << "\n---- Finding first child node named 'Ground' (Hierarchical Lookup) ----" << std::endl;
            auto ground_node = active_tree->findFirstChildNodeByName("Ground");
            if (ground_node) {
                std::cout << "Found Ground node: ID " << ground_node->getId() << " (Status: " << ground_node->getStatus() << ")" << std::endl;
            }

            // 4.5. Demonstrate Tag Lookup (New Feature)
            std::cout << "\n---- Finding all nodes with tag 'Interactable' ----" << std::endl;
            auto interactables = active_tree->findAllNodesByTag("Interactable");
            std::cout << "Found " << interactables.size() << " interactable nodes." << std::endl;
            for (const auto& node : interactables) {
                std::cout << "  - Node: " << node->getName() << " (ID: " << node->getId() << ")" << std::endl;
            }

            // 4.4. Demonstrate Hierarchical Lookup after Detach
            std::cout << "\n---- Detaching PropsRoot (ID: 100) and searching for 'Lamp' again ----" << std::endl;
            auto props_root = active_tree->findNode(100);
            auto env_node_ptr = active_tree->findNode(20);
            if (props_root && env_node_ptr) {
                auto detached_props = active_tree->detach(env_node_ptr, props_root);
                std::cout << "Hierarchical search for 'Lamp' in World Tree: " << (active_tree->findFirstChildNodeByName("Lamp") ? "Found" : "Not Found") << std::endl;
            }

            // 5. Demonstrate multi-parenting
            std::cout << "\n---- Attaching Lamp (ID: 101) directly to Player (ID: 10) as well ----" << std::endl;
            SceneNode* player_node = active_tree->findNode(10);
            if (player_node && lamp_node) {
                // We don't have a tree for the lamp, so we just add the node.
                // This demonstrates the underlying graph structure.
                player_node->addChild(lamp_node->shared_from_this());
                 std::cout << "Lamp is now a child of Player too. Final Tree:" << std::endl;
                active_tree->print();

                // Verify parent counts
                std::cout << "\nLamp node (ID 101) now has " << lamp_node->getParents().size() << " parents." << std::endl;
            }

            // 6. Demonstrate Shared Subtree (DAG) Lifecycle
            std::cout << "\n---- Demonstrating Shared Subtree (DAG) Lifecycle ----" << std::endl;
            
            // Create a shared node "Gem"
            auto gem_node = std::make_shared<SceneNode>(999, "Gem");
            
            // We want to attach this Gem to both Player (ID 10) and Environment (ID 20)
            SceneNode* player_ptr = active_tree->findNode(10);
            SceneNode* env_ptr = active_tree->findNode(20);

            if (player_ptr && env_ptr) {
                // 1. Attach to Player
                std::cout << "Attaching Gem to Player..." << std::endl;
                // We wrap the node in a temporary SceneTree to use the attach API
                auto gem_tree_1 = std::make_unique<SceneTree>(gem_node);
                active_tree->attach(player_ptr, std::move(gem_tree_1));

                // 2. Attach to Environment
                std::cout << "Attaching Gem to Environment..." << std::endl;
                // We wrap the SAME node in another temporary SceneTree
                auto gem_tree_2 = std::make_unique<SceneTree>(gem_node); 
                active_tree->attach(env_ptr, std::move(gem_tree_2));

                active_tree->print();

                // 3. Detach from Player
                std::cout << "Detaching Gem from Player..." << std::endl;
                active_tree->detach(player_ptr, gem_node.get());
                
                // Verify Gem still exists in the tree (via Environment)
                if (active_tree->findNode(999)) {
                    std::cout << "Gem still exists in the tree (reachable via Environment)." << std::endl;
                } else {
                    std::cerr << "Error: Gem was incorrectly removed!" << std::endl;
                }
                
                active_tree->print();
            }

            // 7. Demonstrate Property Listeners
            std::cout << "\n---- Demonstrating Property Listeners ----" << std::endl;
            
            // Register a listener for status changes
            active_tree->addPropertyListener(NodeProperty::Status, [](SceneNode* node, NodeProperty prop, const std::any& oldVal, const std::any& newVal) {
                std::cout << "[Listener] Node '" << node->getName() << "' (ID: " << node->getId() << ") changed status." << std::endl;
            });

            if (player_node) {
                std::cout << "Changing Player status to Inactive..." << std::endl;
                player_node->setStatus(ObjectStatus::Inactive);
            }

            // 8. Demonstrate SceneIO (Save and Load)
            std::cout << "\n---- Demonstrating SceneIO: Saving and Loading ----" << std::endl;
            
            std::filesystem::path dataDir("data");
            if (!std::filesystem::exists(dataDir)) {
                std::filesystem::create_directory(dataDir);
            }
            std::string filename = (dataDir / "example_scene_dump.json").string();
            
            if (SceneIO::saveSceneTree(*active_tree, filename)) {
                std::cout << "Successfully saved scene tree to " << filename << std::endl;
                
                auto loaded_tree = SceneIO::loadSceneTree(filename);
                if (loaded_tree) {
                    std::cout << "Successfully loaded scene tree from " << filename << ". Structure:" << std::endl;
                    loaded_tree->print();
                } else {
                    std::cerr << "Failed to load scene tree." << std::endl;
                }
            } else {
                std::cerr << "Failed to save scene tree." << std::endl;
            }

            // 9. Demonstrate Batching and Update Loop
            std::cout << "\n---- Demonstrating Batching and Update Loop ----" << std::endl;
            
            // Enable batching
            active_tree->setBatchingEnabled(true);
            std::cout << "Batching enabled." << std::endl;

            SceneNode* player_ptr_batch = active_tree->findNode(10);
            if (player_ptr_batch) {
                std::cout << "Renaming Player (ID: 10) to 'Player_Renamed'..." << std::endl;
                player_ptr_batch->setName("Player_Renamed");

                // Check dirty flag
                if (player_ptr_batch->isPropertyDirty(NodeProperty::Name)) {
                    std::cout << "Node is marked dirty (Name property)." << std::endl;
                }

                // Verify immediate lookup fails (because index isn't updated yet)
                if (!active_tree->findNodeByName("Player_Renamed")) {
                    std::cout << "Immediate lookup for 'Player_Renamed' failed (Expected behavior: Index stale)." << std::endl;
                }

                // Simulate Game Loop Update
                std::cout << "Simulating Game Loop Update (calling active_tree->update)..." << std::endl;
                active_tree->update(0.016); // Delta time

                // Verify lookup succeeds after update
                if (active_tree->findNodeByName("Player_Renamed")) {
                    std::cout << "Lookup for 'Player_Renamed' succeeded after update." << std::endl;
                }
            }

            // 10. Demonstrate Versioning Warning
            std::cout << "\n---- Demonstrating Versioning Warning ----" << std::endl;
            std::string future_file = (dataDir / "future_version.json").string();
            {
                std::ofstream ofs(future_file);
                // Create a fake file with version 999
                ofs << "{\"format_version\": 999, \"root\": {\"id\": 500, \"name\": \"FutureNode\", \"status\": \"Active\"}}";
            }

            std::cout << "Loading a file with version 999 (Current is 1)..." << std::endl;
            auto future_tree = SceneIO::loadSceneTree(future_file);
            if (future_tree) {
                std::cout << "Loaded future tree successfully (despite warning). Root ID: " << future_tree->getRoot()->getId() << std::endl;
            }

            // 11. Demonstrate Async Loading and Unloading
            std::cout << "\n---- Demonstrating Async Loading and Unloading ----" << std::endl;
            
            std::string async_file = (dataDir / "async_scene.json").string();
            {
                std::ofstream ofs(async_file);
                ofs << "{\"format_version\": 1, \"root\": {\"id\": 1000, \"name\": \"AsyncNode\", \"status\": \"Active\"}}";
            }

            bool async_done = false;
            std::cout << "Starting async preload of 'AsyncLevel'..." << std::endl;
            manager->preloadSceneAsync("AsyncLevel", async_file, [&](const std::string& name, bool success) {
                std::cout << "[Callback] Preload finished for: " << name << " (Success: " << (success ? "Yes" : "No") << ")" << std::endl;
                async_done = true;
            });

            // Simulate main loop waiting for async tasks
            int loop_count = 0;
            while (!manager->isSceneReady("AsyncLevel") && loop_count < 100) {
                manager->update(); // Must call update to process finished tasks
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (loop_count % 10 == 0) std::cout << "  Waiting for background loading..." << std::endl;
                loop_count++;
            }

            if (manager->isSceneReady("AsyncLevel")) {
                std::cout << "AsyncLevel is ready. Switching now..." << std::endl;
                manager->switchToScene("AsyncLevel");
                std::cout << "Active scene is now: " << manager->getActiveSceneTree()->getRoot()->getName() << std::endl;
                
                std::cout << "Unloading AsyncLevel asynchronously..." << std::endl;
                manager->unloadSceneAsync("AsyncLevel", [](const std::string& name, bool success) {
                    std::cout << "[Callback] Unload finished for: " << name << std::endl;
                });
                
                // Process callbacks for unloading tasks
                for(int i=0; i<5; ++i) {
                    manager->update();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

        } else {
            std::cerr << "Attach failed!" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
