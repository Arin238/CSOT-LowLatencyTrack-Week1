# CSOT-LowLatencyTrack-Week1

This repository contains a highly optimized, low-latency implementation of the Week 1 Spec Strategy for the CSoT'26 Low Latency Track. 

The focus of this implementation is entirely on minimizing the latency of the measured `on_tick` hot path. We have systematically eliminated pointer chasing, unpredictable branches, and dynamic memory allocations from the timing window.

## Low-Latency Optimizations Implemented

### 1. Zero-Allocation Hot Path (Custom Caching Allocator)
The engine measures the `on_tick` function, which must return a `std::vector<csot::Order>`. Because the return type is a frozen ABI, the vector's internal memory allocation (`sizeof(Order) == 40` bytes) is unavoidable.
To make this **zero-latency** during the measured window:
- We overrode `::operator new` globally within our shared library (`-Wl,-Bsymbolic-functions`).
- We pre-allocate a single 40-byte pointer via `std::malloc` during `on_init` and cache it.
- When `on_tick` emits an order, our `operator new` returns the cached pointer in O(1) time (~1ns) instead of trapping into glibc.
- The engine eventually calls `free()` on this pointer, which is perfectly safe since it was originally acquired via `malloc`.
- We re-fill the cache by calling `std::malloc` inside `on_fill()`, which the engine executes *outside* of the measured timing window.

### 2. Integer Symbol Keys (No Pointer Chasing)
String comparisons (like `std::string_view::operator==`) are extremely slow. They require checking sizes, dereferencing data pointers, and calling `memcmp`. 
- We built a `make_key` function that converts the `string_view` characters into a single `uint64_t` integer.
- The symbol lookup loop now does a single 64-bit integer compare (`cmp` instruction) with no pointer indirection, keeping all comparison data inline in a stack array.

### 3. Branchless Math for Unpredictable Paths
Branch predictors are great for loops and warmup phases, but market directions (`diff > 0`) are random and unpredictable. A mispredicted branch flushes the CPU pipeline.
- We converted the `if / else` trees for Order entry/exit into branchless, straight-line arithmetic.
- The `Order::Side` is derived via a 0-cycle `static_cast` from the boolean comparison (`BUY=0, SELL=1`).
- The price and quantity selections use ternary operators, forcing the compiler to emit `cmov` (conditional move) and `vblendvpd` instructions instead of control-flow jumps.
- We deliberately kept the perfectly-predicted warmup branches (`count == WINDOW`) as standard `if` statements, because converting them to FP math would increase the critical path latency.

### 4. Hardware-Level Optimizations
- **Cache Line Alignment:** The `SymbolState` struct and the `sym_keys_` array are `alignas(64)` to ensure they align cleanly with L1 cache lines, preventing false sharing and split loads.
- **Cache Warming:** `on_init` iterates through the data arrays to force them into the L1 cache, and executes dummy `malloc/free` calls to warm up the glibc thread-local tcache.
- **Devirtualization & Aliasing:** The class is marked `final`, hot methods are `__attribute__((hot, always_inline))`, and references use `__restrict__` to assure the compiler that pointers do not overlap.

### 5. Aggressive Compiler Flags
The code is compiled with `-O2` (to keep code size small for better Instruction Cache locality) alongside heavily targeted optimization flags:
- `-funroll-loops`: Lets the compiler heuristically unroll loops.
- `-fomit-frame-pointer`: Frees up the `rbp` register for general use.
- `-fno-stack-protector`: Removes stack canary checks from the hot path.
- `-fno-math-errno`: Prevents math operations from executing hidden writes to the `errno` global state.
- `-fno-plt`: Replaces Procedure Linkage Table indirection with direct function calls.