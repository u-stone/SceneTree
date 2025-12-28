#include <iostream>
#include <stdexcept>
#include "SceneManager.h"

int main() {
    try {
        // 1. Setup SceneManager and Scenes
        auto manager = std::make_unique<SceneManager>();

        // Create a "World" scene
        auto world_scene = std::make_shared<Scene>("World");
        world_scene->addObject(1, "WorldRoot");
        world_scene->addObject(10, "Player");
        world_scene->addObject(20, "Environment");
        world_scene->addObject(21, "Ground");
        world_scene->addObject(22, "Sky");
        world_scene->addObject(23, "Lamp"); // Add a duplicate name to demonstrate lookup features

        // Create a "Props" scene that can be attached
        auto props_scene = std::make_shared<Scene>("Props");
        props_scene->addObject(100, "PropsRoot");
        props_scene->addObject(101, "Lamp");
        props_scene->addObject(102, "Bench");
        
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

        } else {
            std::cerr << "Attach failed!" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
