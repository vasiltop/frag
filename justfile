default:
    @just --list

debug: shaders
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
	cmake --build build

run:
	./build/frag

shaders:
    @mkdir -p build/shaders
    tools/shadercross -t vertex shaders/v.OnlyPosition.hlsl -o build/shaders/v.OnlyPosition.spv
    tools/shadercross -t vertex shaders/v.OnlyPosition.hlsl -o build/shaders/v.OnlyPosition.dxil
    tools/shadercross -t vertex shaders/v.OnlyPosition.hlsl -o build/shaders/v.OnlyPosition.msl
    tools/shadercross -t fragment shaders/f.SolidColor.hlsl -o build/shaders/f.SolidColor.spv
    tools/shadercross -t fragment shaders/f.SolidColor.hlsl -o build/shaders/f.SolidColor.dxil
    tools/shadercross -t fragment shaders/f.SolidColor.hlsl -o build/shaders/f.SolidColor.msl

xcode: shaders
    cmake -B build-xcode -S . -G "Xcode" -DCMAKE_BUILD_TYPE=Debug
    @mkdir -p build-xcode/Debug/shaders
    @cp -R build/shaders/ build-xcode/Debug/shaders/
    open build-xcode/*.xcodeproj
