#!/bin/bash
export CC=clang
export CXX=clang++
rm compile_commands.json
rm -rf build
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cp compile_commands.json ..
cd ..
