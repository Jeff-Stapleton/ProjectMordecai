@echo off
setlocal

echo ============================================
echo  FULL VERIFICATION: Tests + Build
echo ============================================
echo.

echo --- Phase 1: Running Tests ---
call "%~dp0RunTests.bat"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo VERIFICATION FAILED at: Tests
    exit /b 1
)

echo.
echo --- Phase 2: Building Project ---
call "%~dp0Build.bat"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo VERIFICATION FAILED at: Build
    exit /b 2
)

echo.
echo ============================================
echo  VERIFICATION PASSED — Safe to commit
echo ============================================
exit /b 0
