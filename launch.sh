#!/usr/bin/env bash

# This builds the project files and launches Visual Studio
mkdir build
cd build
cmake ..
start dynamic-tree.sln
