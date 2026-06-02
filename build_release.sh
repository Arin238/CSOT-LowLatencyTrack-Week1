#!/bin/bash

set -e

echo "Building Release..."
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target quant_runner bench

echo ""
echo "Running benchmark on synthetic_small.csv..."
./build-release/bench --benchmark_filter=BM_SpecStrategy_on_tick
