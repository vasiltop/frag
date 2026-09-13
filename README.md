# frag

A 3D game engine built on top of **SDL3**.

## Build

**Windows**

```bat
build.bat
```

**macOS / Linux**

```sh
./build.sh
```

**Xcode**

```sh
./build-xcode.sh
```

## Run

Windows: `.\build\Debug\frag.exe`

macOS / Linux: `./build/frag`

## Test

```sh
cmake --build build --target frag_tests
```

Windows: `.\build\Debug\frag_tests.exe`

macOS / Linux: `./build/frag_tests`
