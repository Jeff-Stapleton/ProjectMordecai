@echo off
setlocal

set ENGINE_DIR=C:\Users\jeffd\Documents\Gamedev\UnrealEngine
set PROJECT_DIR=C:\Users\jeffd\Documents\Gamedev\ProjectMordecai
set PROJECT_FILE=%PROJECT_DIR%\ProjectMordecai.uproject

echo ============================================
echo  Building ProjectMordecai (Development Editor, Win64)
echo ============================================
echo.

call "%ENGINE_DIR%\Engine\Build\BatchFiles\Build.bat" LyraEditor Win64 Development -Project="%PROJECT_FILE%" -WaitMutex -FromMsBuild

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo  BUILD FAILED (exit code %ERRORLEVEL%)
    echo ============================================
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================
echo  BUILD SUCCEEDED
echo ============================================
exit /b 0
