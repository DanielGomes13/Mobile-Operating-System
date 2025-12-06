#!/bin/sh
set -e
make clean
make all
echo "Build complete: kernel.img"
