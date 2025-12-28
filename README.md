# SceneTree

A C++ Scene Management System using a Directed Acyclic Graph (DAG) to represent scene hierarchies.

This project provides a flexible scene management solution for game development, allowing complex scene compositions where a single scene can be part of multiple parent scenes.

## Features

- **Hierarchical Scene Structure**: `SceneTree` represents a scene graph.
- **Multi-parenting**: A `SceneTree` can be attached to multiple parents, forming a DAG.
- **Fast Lookups**:
  - **By ID**: O(1) lookup for unique IDs.
  - **By Name**: O(1) lookup for node names, supporting duplicate names and scoped searches within subtrees.
- **Resource Management**: Efficiently attach/detach scenes, simulating resource loading/unloading.
- **Scene Switching**: A `SceneManager` handles high-level scene transitions.
- **C++17**: Implemented in modern C++.
- **CMake**: Cross-platform build management.
- **Google Test**: Includes unit tests for verification.

## Project Structure

```
/
├── src/            # Core library source code
├── doc/            # Design documentation
├── examples/       # Usage examples
├── external/       # Third-party libraries (e.g., Google Test via FetchContent)
├── tests/          # Unit tests
├── CMakeLists.txt  # Main build script
├── README.md       # This file
└── build.bat       # Windows build script
```

## Prerequisites

- CMake (version 3.15 or higher)
- A C++17 compatible compiler (e.g., MSVC, GCC, Clang)

## How to Build and Run

### Windows

A convenient `build.bat` script is provided to automate the build process. Simply run it from the project root directory:

```bash
.\build.bat
```

This will:
1. Create a `build` directory.
2. Run CMake to configure the project and generate build files.
3. Build the project (library, example, and tests).
4. Run the tests.
5. Run the example application.

### Manual Build (All Platforms)

1.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```

2.  **Configure the project with CMake:**
    ```bash
    # Replace "Visual Studio 17 2022" with your desired generator if needed.
    cmake .. -G "Visual Studio 17 2022" 
    ```

3.  **Build the project:**
    ```bash
    cmake --build . --config Release
    ```

4.  **Run the tests (optional):**
    ```bash
    ctest -C Release --output-on-failure
    ```

5.  **Run the example:**
    ```bash
    # The executable will be in the examples subdirectory of the build directory
    ./examples/Release/scene_management_example.exe
    ```
