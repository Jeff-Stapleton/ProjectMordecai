@echo off
setlocal

rem Delegates to run_tests.py, which judges pass/fail from the test log.
rem UnrealEditor-Cmd exits 0 even when tests fail, so its exit code can't be trusted.

set TEST_FILTER=%1
if "%TEST_FILTER%"=="" set TEST_FILTER=Mordecai

echo ============================================
echo  Running Automation Tests: %TEST_FILTER%
echo ============================================
echo.

py "%~dp0run_tests.py" %TEST_FILTER%

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
