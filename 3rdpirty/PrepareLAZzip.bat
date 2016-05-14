@echo off

set CURDIR=%CD%

cd /d %~dp0

set PLATFORM=Win32

set BUILD_DIR=LASzip\%PLATFORM%

if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
)

if not exist %BUILD_DIR%\laszip.sln (
    cd %BUILD_DIR%
    if %PLATFORM% == Win32 (
        cmake.exe -G "Visual Studio 12 2013" ..\
    ) else (
        cmake.exe -G "Visual Studio 12 2013 Win64" ..\
    )
    cd ..\..\
)

cd /d %CURDIR%

exit /b 1

:error
cd /d %CURDIR%
echo "Error====="
pause
exist /b 0