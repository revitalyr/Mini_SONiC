@echo off
echo ========================================
echo Mini SONiC Real Demo Builder & Runner
echo ========================================
echo.

REM Check if main project is built
if not exist "..\build\mini_sonic_lib.lib" (
    echo [ERROR] Main Mini SONiC library not found!
    echo Please build the main project first:
    echo   cd .. 
    echo   cmake --build build --config Release
    echo.
    exit /b 1
)

echo [INFO] Main library found, proceeding with demo build...
echo.

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo [INFO] Configuring demo with CMake...
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DMINI_SONIC_BUILD_DIR=..\..\build ..
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

echo [INFO] Building demo...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.

REM Check if demo executable exists
if not exist "mini_sonic_real_demo.exe" (
    echo [ERROR] Demo executable not found!
    exit /b 1
)

echo Available demo configurations:
echo   1. Animated Real Demo (BEST) - Real components with live animation
echo   2. Clean Visual Demo - Clean output with basic animation
echo   3. Real Demo (verbose) - Detailed logging output
echo   4. Animated Demo - Custom configuration
echo   5. Show help
echo.

set /p choice="Select demo configuration (1-5): "

if "%choice%"=="1" (
    echo.
    echo [INFO] Running Animated Real Demo (BEST)...
    echo ========================================
    mini_sonic_animated_demo.exe --switches 3 --rate 100 --duration 30
) else if "%choice%"=="2" (
    echo.
    echo [INFO] Running Clean Visual Demo...
    echo ========================================
    mini_sonic_clean_demo.exe --switches 3 --rate 100 --duration 30
) else if "%choice%"=="3" (
    echo.
    echo [INFO] Running Real Demo (verbose)...
    echo ========================================
    mini_sonic_real_demo.exe --switches 3 --rate 50 --duration 15
) else if "%choice%"=="4" (
    echo.
    set /p switches="Number of switches (default 3): "
    set /p rate="Packets per second (default 100): "
    set /p duration="Duration in seconds (default 30): "
    
    if "%switches%"=="" set switches=3
    if "%rate%"=="" set rate=100
    if "%duration%"=="" set duration=30
    
    echo.
    echo [INFO] Running Animated Custom Demo...
    echo ========================================
    mini_sonic_animated_demo.exe --switches %switches% --rate %rate% --duration %duration%
) else if "%choice%"=="5" (
    echo.
    echo [INFO] Showing help for all demos...
    echo ========================================
    echo.
    echo Animated Real Demo:
    mini_sonic_animated_demo.exe --help
    echo.
    echo Clean Visual Demo:
    mini_sonic_clean_demo.exe --help
    echo.
    echo Real Demo (verbose):
    mini_sonic_real_demo.exe --help
) else (
    echo [WARNING] Invalid choice, running animated demo...
    echo.
    echo [INFO] Running Animated Real Demo...
    echo ========================================
    mini_sonic_animated_demo.exe
)

echo.
echo ========================================
echo Demo completed!
echo ========================================
echo Check demo_output.log for detailed results.
echo.

REM Ask if user wants to open the log file
set /p open_log="Open demo log file? (y/n): "
if /i "%open_log%"=="y" (
    notepad demo_output.log
)
