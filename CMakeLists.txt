cmake_minimum_required(VERSION 3.20)
project(csot_quant_platform CXX)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(
    -Wall -Wextra -Wpedantic
)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(
    quant_runner
    src/main.cpp
    src/engine.cpp
    strategies/spec_strategy.cpp
)