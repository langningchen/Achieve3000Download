#!/bin/fish
echo -e "\033[33m"
echo "Achieve3000Download  Copyright (C) 2023  langningchen"
echo "This program comes with ABSOLUTELY NO WARRANTY."
echo "This is free software, and you are welcome to redistribute it under certain conditions."
echo -e "\033[0m"
cmake -B build
cmake --build build
set Username (cat Username)
set Password (cat Password)
./build/main -u $Username -p $Password
