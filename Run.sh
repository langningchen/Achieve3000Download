#!/bin/fish
cmake -B build
cmake --build build
set Username (cat Username)
set Password (cat Password)
./build/main -u $Username -p $Password
