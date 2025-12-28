#include <iostream>
#include <stdexcept>
#include "SceneManager.h"

void print_tree(const SceneNode& node, int depth = 0) {
    std::string indent(depth * 4, ' ');
    std::cout << indent << "- " << node.getName() << " (ID: " << node.getId() << ", Status: " << node.getStatus() 
              << ", Parents: " << node.getParents().size() << ")" << std::endl;
    for (const auto& child : node.getChildren()) {
        print_tree(*child, depth + 1);
    }
}

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
            print_tree(*active_tree->getRoot());
        }

        // 3. Attach the props scene to the environment node
        std::cout << "\n---- Attaching Props Scene to Environment (ID: 20) ----" << std::endl;
        bool attached = manager->attachScene("World", "Props", 20);

        if (attached && active_tree) {
            std::cout << "Attach successful. Updated Scene Tree:" << std::endl;
            print_tree(*active_tree->getRoot());

            // 4. Find a node and update its status
            std::cout << "\n---- Finding Lamp (ID: 101) and setting status to 'broken' ----" << std::endl;
            SceneNode* lamp_node = active_tree->findNode(101);
            if (lamp_node) {
                lamp_node->setStatus("broken");
                std::cout << "Lamp status updated. Final Tree:" << std::endl;
                print_tree(*active_tree->getRoot());
            } else {
                std::cerr << "Could not find lamp node!" << std::endl;
            }

            // 5. Demonstrate multi-parenting
            std::cout << "\n---- Attaching Lamp (ID: 101) directly to Player (ID: 10) as well ----" << std::endl;
            SceneNode* player_node = active_tree->findNode(10);
            if (player_node && lamp_node) {
                // We don't have a tree for the lamp, so we just add the node.
                // This demonstrates the underlying graph structure.
                player_node->addChild(lamp_node->shared_from_this());
                 std::cout << "Lamp is now a child of Player too. Final Tree:" << std::endl;
                print_tree(*active_tree->getRoot());

                // Verify parent counts
                std::cout << "\nLamp node (ID 101) now has " << lamp_node->getParents().size() << " parents." << std::endl;
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
