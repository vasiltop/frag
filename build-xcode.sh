#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

if [ ! -x tools/shadercross ]; then
    echo "error: tools/shadercross not found or not executable" >&2
    exit 1
fi

mkdir -p build/shaders
tools/shadercross -t vertex shaders/v.hlsl -o build/shaders/v.spv
tools/shadercross -t vertex shaders/v.hlsl -o build/shaders/v.dxil
tools/shadercross -t vertex shaders/v.hlsl -o build/shaders/v.msl
tools/shadercross -t fragment shaders/f.hlsl -o build/shaders/f.spv
tools/shadercross -t fragment shaders/f.hlsl -o build/shaders/f.dxil
tools/shadercross -t fragment shaders/f.hlsl -o build/shaders/f.msl

cmake -B build-xcode -S . -G "Xcode" -DCMAKE_BUILD_TYPE=Debug
mkdir -p build-xcode/Debug/shaders
cp -R build/shaders/ build-xcode/Debug/shaders/
cp -R assets/ build-xcode/Debug/assets/
open build-xcode/*.xcodeproj
