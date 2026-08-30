default:
    @just --list

build:
	cmake -B build -S .
	cmake --build build

run:
	./build/frag
