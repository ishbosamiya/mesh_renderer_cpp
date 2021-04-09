#!/bin/bash

echo "initializing git submodules"
git submodule init

echo "updating git submodules"
git submodule update

cd deps/
mkdir temp
cd temp
git clone https://github.com/Dav1dde/glad.git --depth=1
cd glad
python3 -m glad --generator c-debug --out-path ../../glad
cd ../..
rm -r temp/
