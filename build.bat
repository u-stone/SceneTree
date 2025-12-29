@echo off
setlocal enabledelayedexpansion

:: Define vswhere path
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=vswhere.exe"

:: Normalize slashes to backslashes to avoid parsing issues
set "VSWHERE=%VSWHERE:/=\%"

echo VSWHERE path: %VSWHERE%

set "GENERATOR="
set "ASAN_SUPPORTED=OFF"

echo [INFO] Searching for Visual Studio installation...

:: Prioritize searching for VS 2019 (v16)
for /f "usebackq tokens=*" %%i in (`^""%VSWHERE%" -latest -version "[16.0,17.0)" -property installationPath^"`) do (
    set "GENERATOR=Visual Studio 16 2019"
    REM Check if ASan component is installed
    for /f "usebackq tokens=*" %%j in (`^""%VSWHERE%" -latest -version "[16.0,17.0)" -requires Microsoft.VisualStudio.Component.VC.ASAN -property installationPath^"`) do (
        set "ASAN_SUPPORTED=ON"
    )
)

:: If VS 2019 is not found, search for VS 2022 (v17)
if not defined GENERATOR (
    for /f "usebackq tokens=*" %%i in (`^""%VSWHERE%" -latest -version "[17.0,18.0)" -property installationPath^"`) do (
        set "GENERATOR=Visual Studio 17 2022"
        REM Check if ASan component is installed
        for /f "usebackq tokens=*" %%j in (`^""%VSWHERE%" -latest -version "[17.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.ASAN -property installationPath^"`) do (
            set "ASAN_SUPPORTED=ON"
        )
    )
)

if not defined GENERATOR (
    echo [ERROR] Visual Studio 2022 or 2019 not found.
    exit /b 1
)

echo [INFO] ASan Supported: %ASAN_SUPPORTED%
:: Handle ASan enablement logic
set "USE_ASAN_FLAG=OFF"
if /I "%1"=="asan" (
    echo [INFO] ASan build requested.
    if "%ASAN_SUPPORTED%"=="ON" (
        set "USE_ASAN_FLAG=ON"
        echo [INFO] Address Sanitizer ^(ASan^) support detected and enabled.
    ) else (
        echo [WARN] ASan requested, but the required MSVC component is not installed.
    )
)

echo [INFO] Using Generator: %GENERATOR%

REM Remove existing cache to prevent generator/platform conflicts
if exist build\CMakeCache.txt del /q build\CMakeCache.txt

echo [INFO] Configuring and building project...
cmake -G "%GENERATOR%" -A x64 -DUSE_ASAN=%USE_ASAN_FLAG% -S . -B build
@REM cmake --build build --config Debug

REM Change to the build directory
cd build

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
