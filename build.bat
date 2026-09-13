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

echo Generating compile_commands.json for LSP
cmake -B build-lsp -G Ninja -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DFETCHCONTENT_SOURCE_DIR_SDL3="%CD%\build\_deps\sdl3-src" -DFETCHCONTENT_SOURCE_DIR_GLM="%CD%\build\_deps\glm-src"
if errorlevel 1 (
    echo warning: could not generate compile_commands.json. Install Ninja and rerun.
) else (
    copy /Y build-lsp\compile_commands.json compile_commands.json >nul
)
