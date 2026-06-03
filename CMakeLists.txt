cmake_minimum_required(VERSION 3.20)
project(csot_quant_platform CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native -flto")

add_compile_options(
    -Wall -Wextra -Wpedantic
)

include_directories(${CMAKE_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.2
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "Disable benchmark GTest integration")
FetchContent_MakeAvailable(benchmark)

add_executable(
    quant_runner
    src/main.cpp
    src/engine.cpp
    strategies/spec_strategy.cpp
)

add_executable(
    bench
    bench/bench.cpp
    strategies/spec_strategy.cpp
)

target_link_libraries(bench PRIVATE benchmark::benchmark)

target_compile_definitions(bench PRIVATE BENCH_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
