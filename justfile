default:
    @just --list

debug: shaders
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
	cmake --build build

run:
	./build/frag

test:
	cmake --build build --target frag_tests
	./build/frag_tests

shaders:
    @mkdir -p build/shaders
    tools/shadercross -t vertex shaders/v.hlsl -o build/shaders/v.spv
    tools/shadercross -t vertex shaders/v.hlsl -o build/shaders/v.dxil
    tools/shadercross -t vertex shaders/v.hlsl -o build/shaders/v.msl
    tools/shadercross -t fragment shaders/f.hlsl -o build/shaders/f.spv
    tools/shadercross -t fragment shaders/f.hlsl -o build/shaders/f.dxil
    tools/shadercross -t fragment shaders/f.hlsl -o build/shaders/f.msl

xcode: shaders
    cmake -B build-xcode -S . -G "Xcode" -DCMAKE_BUILD_TYPE=Debug
    @mkdir -p build-xcode/Debug/shaders
    @cp -R build/shaders/ build-xcode/Debug/shaders/
    @cp -R assets/ build-xcode/Debug/assets/
    open build-xcode/*.xcodeproj
