#!/bin/bash

set -e

mkdir -p build-emscripten

mkdir -p build-emscripten/datapacktool
pushd "tools/datapack"
mkdir -p ../../build-emscripten/datapacktool
cmake -B ../../build-emscripten/datapacktool -G "Ninja"
cmake --build ../../build-emscripten/datapacktool
pushd
