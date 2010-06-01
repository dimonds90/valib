@echo off

set TEST=test2.exe

if "%1" == "" goto usage
if not exist "..\..\samples\test" goto err_no_samples
if not exist "%1\%TEST%" goto err_config

set CONFIG=%1
shift
shift
set TESTS=%*

cd ..\..\samples\test
..\..\valib\test2\%CONFIG%\%TEST% %TESTS%
cd ..\..\valib\test2
goto end


:usage
echo Run the test suite. Usage:
echo   test conifg [test] [test]
echo where
echo   config - configuration to test: Debug, Release, x64\Debug or x64\Release
echo   test   - test to execute; you can specify several tests
echo.
echo You must have the samples folder. Directory layout must look like:
echo ..\samples
echo ..\samples\test
echo ..\valib
echo ..\valib\test
echo ..\valib\text\%%config%%
goto fail

:err_config
echo Cannot find %1\%TEST%
goto fail

:err_no_samples
echo Error: No samples folder found!
goto fail

:fail
fail >nul 2>&1

:end
