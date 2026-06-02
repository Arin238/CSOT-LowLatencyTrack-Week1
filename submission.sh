#!/bin/bash
set -e
cd "$(dirname "$0")"

echo "Building submission shared object..."
g++ -std=c++17 -O2 -march=native -fPIC -shared \
    -Iinclude \
    strategies/spec_strategy.cpp \
    -o libspec_strategy.so

echo "Built libspec_strategy.so"
ls -lh libspec_strategy.so
