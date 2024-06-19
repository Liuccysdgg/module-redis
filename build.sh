#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

sudo apt-get install libhiredis-dev -y
mkdir target -p
mkdir build -p
cd build
cmake ..
make
cp -f libredis.so ../target
