@echo off
setlocal

set ENGINE_DIR=C:\Users\jeffd\Documents\Gamedev\UnrealEngine
set PROJECT_DIR=C:\Users\jeffd\Documents\Gamedev\ProjectMordecai
set PROJECT_FILE=%PROJECT_DIR%\ProjectMordecai.uproject
set RUNUAT=%ENGINE_DIR%\Engine\Build\BatchFiles\RunUAT.bat
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Development
set ARCHIVE_DIR=%PROJECT_DIR%\PackagedBuilds

echo ============================================
echo  Packaging ProjectMordecai (Win64, %CONFIG%)
echo ============================================
echo  Output: %ARCHIVE_DIR%
echo ============================================
echo.

call "%RUNUAT%" BuildCookRun ^
    -project="%PROJECT_FILE%" ^
    -noP4 ^
    -platform=Win64 ^
    -clientconfig=%CONFIG% ^
    -build ^
    -cook ^
    -allmaps ^
    -stage ^
    -pak ^
    -archive ^
    -archivedirectory="%ARCHIVE_DIR%" ^
    -unattended ^
    -utf8output

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo  PACKAGING FAILED (exit code %ERRORLEVEL%)
    echo ============================================
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================
echo  Zipping build...
echo ============================================

:: Build a timestamped zip name
for /f "tokens=2 delims==" %%a in ('wmic os get localdatetime /value') do set DT=%%a
set TIMESTAMP=%DT:~0,8%-%DT:~8,6%
set ZIP_NAME=ProjectMordecai-%CONFIG%-Win64-%TIMESTAMP%.zip
set ZIP_PATH=%ARCHIVE_DIR%\%ZIP_NAME%

powershell -NoProfile -Command "Compress-Archive -Path '%ARCHIVE_DIR%\Windows\*' -DestinationPath '%ZIP_PATH%' -Force"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================
    echo  ZIP FAILED — staged build is still at:
    echo  %ARCHIVE_DIR%\Windows\
    echo ============================================
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================
echo  PACKAGING SUCCEEDED
echo  Staged: %ARCHIVE_DIR%\Windows\
echo  Zipped: %ZIP_PATH%
echo ============================================
exit /b 0
