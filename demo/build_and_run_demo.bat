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
echo   1. Windows Visual Demo (BEST) - Pure ASCII, Windows compatible
echo   2. Enhanced Visual Demo - Real network visualization, ASCII compatible
echo   3. Visual TUI Demo - Real network visualization with Unicode
echo   4. Pure TUI Demo - Easy to see TUI, no dependencies
echo   5. Basic TUI Demo - Easy to see TUI animation
echo   6. Clean Animated Demo (REAL) - Real components, ZERO debug output
echo   7. Silent Animated Demo - Real components, minimal logging
echo   8. Animated Real Demo - Real components with live animation
echo   9. Clean Visual Demo - Clean output with basic animation
echo   10. Real Demo (verbose) - Detailed logging output
echo   11. Custom Windows Visual Demo - Windows visual, custom settings
echo   12. Show help
echo.

set /p choice="Select demo configuration (1-12): "

if "%choice%"=="1" (
    echo.
    echo [INFO] Running Windows Visual Demo (BEST - Pure ASCII, Windows compatible)...
    echo ========================================
    mini_sonic_windows_visual_demo.exe --switches 3 --rate 2 --duration 30
) else if "%choice%"=="2" (
    echo.
    echo [INFO] Running Enhanced Visual Demo (ASCII compatible)...
    echo ========================================
    mini_sonic_enhanced_visual_demo.exe --switches 3 --rate 2 --duration 30
) else if "%choice%"=="3" (
    echo.
    echo [INFO] Running Visual TUI Demo (Unicode visualization)...
    echo ========================================
    mini_sonic_visual_tui_demo.exe --switches 3 --rate 2 --duration 30
) else if "%choice%"=="4" (
    echo.
    echo [INFO] Running Pure TUI Demo (Easy to see, no deps)...
    echo ========================================
    mini_sonic_pure_tui_demo.exe --switches 3 --rate 1 --duration 30
) else if "%choice%"=="5" (
    echo.
    echo [INFO] Running Basic TUI Demo (Easy to see animation)...
    echo ========================================
    mini_sonic_basic_tui_demo.exe --switches 3 --rate 1 --duration 30
) else if "%choice%"=="6" (
    echo.
    echo [INFO] Running Clean Animated Demo (REAL - ZERO Debug Output)...
    echo ========================================
    mini_sonic_clean_anim_demo.exe --switches 3 --rate 15 --duration 25
) else if "%choice%"=="7" (
    echo.
    echo [INFO] Running Silent Animated Demo...
    echo ========================================
    mini_sonic_silent_demo.exe --switches 3 --rate 30 --duration 30
) else if "%choice%"=="8" (
    echo.
    echo [INFO] Running Animated Real Demo...
    echo ========================================
    mini_sonic_animated_demo.exe --switches 3 --rate 100 --duration 30
) else if "%choice%"=="9" (
    echo.
    echo [INFO] Running Clean Visual Demo...
    echo ========================================
    mini_sonic_clean_demo.exe --switches 3 --rate 100 --duration 30
) else if "%choice%"=="10" (
    echo.
    echo [INFO] Running Real Demo (verbose)...
    echo ========================================
    mini_sonic_real_demo.exe --switches 3 --rate 50 --duration 15
) else if "%choice%"=="11" (
    echo.
    set /p switches="Number of switches (default 3): "
    set /p rate="Packets per second (default 2): "
    set /p duration="Duration in seconds (default 30): "
    set /p refresh="Refresh interval in ms (default 1500): "
    
    if "%switches%"=="" set switches=3
    if "%rate%"=="" set rate=2
    if "%duration%"=="" set duration=30
    if "%refresh%"=="" set refresh=1500
    
    echo.
    echo [INFO] Running Custom Windows Visual Demo...
    echo ========================================
    mini_sonic_windows_visual_demo.exe --switches %switches% --rate %rate% --duration %duration% --refresh %refresh%
) else if "%choice%"=="12" (
    echo.
    echo [INFO] Showing help for all demos...
    echo ========================================
    echo.
    echo Windows Visual Demo:
    mini_sonic_windows_visual_demo.exe --help
    echo.
    echo Enhanced Visual Demo:
    mini_sonic_enhanced_visual_demo.exe --help
    echo.
    echo Visual TUI Demo:
    mini_sonic_visual_tui_demo.exe --help
    echo.
    echo Pure TUI Demo:
    mini_sonic_pure_tui_demo.exe --help
    echo.
    echo Basic TUI Demo:
    mini_sonic_basic_tui_demo.exe --help
    echo.
    echo Clean Animated Demo:
    mini_sonic_clean_anim_demo.exe --help
    echo.
    echo Silent Animated Demo:
    mini_sonic_silent_demo.exe --help
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
    echo [WARNING] Invalid choice, running Windows visual demo...
    echo.
    echo [INFO] Running Windows Visual Demo...
    echo ========================================
    mini_sonic_windows_visual_demo.exe
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
