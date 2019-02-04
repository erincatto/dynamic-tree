#!/usr/bin/env bash

# This builds the project files and launches Visual Studio
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" ..
start dynamic-tree.sln
