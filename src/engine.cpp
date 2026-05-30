#include "../include/engine.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <chrono>

void Engine::load_ticks(const std::string& path) {
    std::ifstream file(path);

    std::string line;

    // skip header
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);

        std::string token;

        auto st = std::make_unique<StoredTick>();

        std::getline(ss, token, ',');
        st->tick.timestamp_ns = std::stoull(token);

        std::getline(ss, token, ',');
        st->backing = token;
        st->tick.symbol = std::string_view(st->backing);

        std::getline(ss, token, ',');
        st->tick.bid_px = std::stod(token);

        std::getline(ss, token, ',');
        st->tick.ask_px = std::stod(token);

        std::getline(ss, token, ',');
        st->tick.bid_qty = std::stoul(token);

        std::getline(ss, token, ',');
        st->tick.ask_qty = std::stoul(token);

        ticks.push_back(std::move(st));
    }

    std::cout << "Loaded " << ticks.size() << " ticks\n";
    
    if (!ticks.empty()) {
        const auto& first = *ticks.front();
        const auto& last = *ticks.back();

        std::cout << "First tick:\n";
        std::cout << first.tick.symbol << " "
                  << first.tick.bid_px << " "
                  << first.tick.ask_px << "\n";

        std::cout << "Last tick:\n";
        std::cout << last.tick.symbol << " "
                  << last.tick.bid_px << " "
                  << last.tick.ask_px << "\n";
    }
}

void Engine::run(csot::Strategy& strategy) {
    strategy.on_init();

    for (const auto& stptr : ticks) {
        const csot::Tick& t = stptr->tick;

        auto t0 = std::chrono::steady_clock::now();
        auto orders = strategy.on_tick(t);
        auto t1 = std::chrono::steady_clock::now();

        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        hist_.record(static_cast<std::uint64_t>(ns));

        if (!orders.empty()) {
            // Week-1 engine: every order fills immediately.
            const csot::Order& o = orders.front();
            strategy.on_fill(o, o.price, o.qty);
        }
    }

    std::cout << "Latency histogram:\n";
    hist_.print(std::cout);
}