@echo OFF
cd build
cmake --build .
ctest -V
cd ..
