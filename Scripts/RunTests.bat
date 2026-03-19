@echo off
setlocal

set ENGINE_DIR=C:\Users\jeffd\Documents\Gamedev\UnrealEngine
set PROJECT_DIR=C:\Users\jeffd\Documents\Gamedev\ProjectMordecai
set PROJECT_FILE=%PROJECT_DIR%\ProjectMordecai.uproject
set TEST_FILTER=%1
if "%TEST_FILTER%"=="" set TEST_FILTER=Mordecai

echo ============================================
echo  Running Automation Tests: %TEST_FILTER%
echo ============================================
echo.

"%ENGINE_DIR%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_FILE%" -ExecCmds="Automation RunTests %TEST_FILTER%" -Unattended -NullRHI -NoSound -NoSplash -Log -LogCmds="LogAutomationTest Display" -TestExit="Automation Test Queue Empty"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo  TESTS FAILED (exit code %ERRORLEVEL%)
    echo ============================================
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================
echo  ALL TESTS PASSED
echo ============================================
exit /b 0
