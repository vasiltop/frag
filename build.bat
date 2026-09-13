@echo off
setlocal
cd /d "%~dp0"

if exist tools\shadercross.exe (
    set "SHADERCROSS=tools\shadercross.exe"
) else if exist tools\shadercross (
    set "SHADERCROSS=tools\shadercross"
) else (
    echo error: tools\shadercross.exe not found
    exit /b 1
)

if not exist build\shaders mkdir build\shaders
%SHADERCROSS% -t vertex shaders\v.hlsl -o build\shaders\v.spv
if errorlevel 1 exit /b 1
%SHADERCROSS% -t vertex shaders\v.hlsl -o build\shaders\v.dxil
if errorlevel 1 exit /b 1
%SHADERCROSS% -t vertex shaders\v.hlsl -o build\shaders\v.msl
if errorlevel 1 exit /b 1
%SHADERCROSS% -t fragment shaders\f.hlsl -o build\shaders\f.spv
if errorlevel 1 exit /b 1
%SHADERCROSS% -t fragment shaders\f.hlsl -o build\shaders\f.dxil
if errorlevel 1 exit /b 1
%SHADERCROSS% -t fragment shaders\f.hlsl -o build\shaders\f.msl
if errorlevel 1 exit /b 1

if not exist build\Debug\shaders mkdir build\Debug\shaders
xcopy /Y /I /Q build\shaders\* build\Debug\shaders\
if errorlevel 1 exit /b 1

cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 exit /b 1
cmake --build build --config Debug
if errorlevel 1 exit /b 1
