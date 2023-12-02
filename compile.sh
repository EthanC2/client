#!/bin/bash

# Variables
COMPILER=g++
VERSION='-std=c++2a'
WARNINGS='-Wextra -Wall'
LIBRARIES='-lncurses'

# Compile code
rm -v ./build/client
$COMPILER $VERSION $WARNINGS $@ ./src/*.cpp -o ./build/client $LIBRARIES