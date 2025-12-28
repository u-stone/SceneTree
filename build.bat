@echo off
setlocal

REM Create a build directory if it doesn't exist
if not exist build (
    echo ---- Creating build directory ----
    mkdir build
)

REM Change to the build directory
cd build

REM Run CMake to configure the project. 
REM This generates Visual Studio project files by default on most Windows systems with VS installed.
echo ---- Configuring project with CMake ----
cmake .. -G "Visual Studio 17 2022" -DUSE_ASAN=ON
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    goto :eof
)

REM Build the project (both Debug and Release configurations)
echo ---- Building project (Release) ----
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Release build failed.
    goto :eof
)

REM Run the tests
echo ---- Running tests ----
ctest -C Release --output-on-failure
if %errorlevel% neq 0 (
    echo Tests failed.
    goto :eof
)

REM Run the example
echo ---- Running example ----
.\examples\Release\scene_management_example.exe

echo ---- Build and run successful ----

endlocal
