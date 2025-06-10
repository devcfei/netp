!/bin/bash


cmake -S . -B build/Debug -DCMAKE_INSTALL_PREFIX=install/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug --target install