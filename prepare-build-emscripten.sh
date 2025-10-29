#!/bin/bash

set -e

mkdir -p build-emscripten
git clone --recurse https://github.com/shader-slang/slang build-emscripten/slang

pushd "build-emscripten/slang"
mkdir -p build
cmake -B build -G "Ninja" -DSLANG_SLANG_LLVM_FLAVOR=DISABLE
cmake --build build
cmake --install build
popd

mkdir -p build-emscripten/datapacktool
pushd "tools/datapack"
mkdir -p ../../build-emscripten/datapacktool
cmake -B ../../build-emscripten/datapacktool -G "Ninja"
cmake --build ../../build-emscripten/datapacktool
pushd
