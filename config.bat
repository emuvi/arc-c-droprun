@echo OFF
set CC=clang
set CXX=clang++
del compile_commands.json
rmdir /S /Q build
mkdir build
cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1
copy compile_commands.json ..
cd ..
