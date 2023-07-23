#!/usr/bin/env bash

# This builds the project files and launches Visual Studio
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
start dynamic-tree.sln
