#include <benchmark/benchmark.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "strategy.hpp"

static std::vector<csot::Tick> load_ticks(const std::string& path) {
    std::vector<csot::Tick> ticks;
    std::ifstream file(path);
    std::string line;
    std::getline(file, line); // header

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;
        csot::Tick t;

        std::getline(ss, token, ',');
        t.timestamp_ns = std::stoull(token);

        std::getline(ss, token, ',');
        t.symbol = token;

        std::getline(ss, token, ',');
        t.bid_px = std::stod(token);

        std::getline(ss, token, ',');
        t.ask_px = std::stod(token);

        std::getline(ss, token, ',');
        t.bid_qty = std::stoul(token);

        std::getline(ss, token, ',');
        t.ask_qty = std::stoul(token);

        ticks.push_back(std::move(t));
    }
    return ticks;
}

static void BM_SpecStrategy_on_tick(benchmark::State& state) {
    const std::string path = std::string(BENCH_DATA_DIR) + "/synthetic_small.csv";
    const auto ticks = load_ticks(path);
    csot::Strategy* strategy = create_strategy();
    strategy->on_init();

    for (auto _ : state) {
        for (const auto& t : ticks) {
            auto orders = strategy->on_tick(t);
            if (!orders.empty()) {
                const auto& o = orders.front();
                strategy->on_fill(o, o.price, o.qty);
                benchmark::DoNotOptimize(o);
            }
        }
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(ticks.size()));
    delete strategy;
}

BENCHMARK(BM_SpecStrategy_on_tick)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
