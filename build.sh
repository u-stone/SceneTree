#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Check for generator mismatch. If the existing cache was not created with Xcode,
# we must remove it to avoid CMake errors when switching generators.
if [ -f "build/CMakeCache.txt" ]; then
    if ! grep -q "CMAKE_GENERATOR:INTERNAL=Xcode" "build/CMakeCache.txt"; then
        echo "---- Generator mismatch detected. Cleaning build directory... ----"
        rm -rf build
    fi
fi

# Create build directory
mkdir -p build

# Navigate to the build directory
cd build

# Configure the project with CMake using the Xcode generator
echo "---- Configuring project with CMake (Xcode) ----"
cmake .. -G Xcode -DUSE_ASAN=ON > /dev/null

# Build the project (Release configuration)
echo "---- Building project (Release) ----"
cmake --build . --config Release > /dev/null

# Run tests
echo "---- Running tests ----"
ctest -C Release --output-on-failure

# Run the example
echo "---- Running example ----"
# Note: On Unix systems, executables usually don't have an .exe extension
# Xcode is a multi-config generator, so it will place the binary in a Release subdirectory
if [ -f "./examples/Release/scene_management_example" ]; then
    ./examples/Release/scene_management_example
elif [ -f "./examples/scene_management_example" ]; then
    ./examples/scene_management_example
fi

echo "---- Build and run successful ----"