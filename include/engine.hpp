#pragma once

#include <string>
#include <vector>
#include <memory>
#include "histogram.hpp"
#include "strategy.hpp"

namespace csot { struct Tick; }

// Internal storage for ticks: we own the symbol string data so that
// `std::string_view` fields in `csot::Tick` remain valid while the engine
// replays ticks. We store everything in a flat contiguous vector (pre-reserved)
// for maximum cache locality during replay.
struct StoredTick {
    csot::Tick tick;
    std::string backing;
};

class Engine {
public:
    void load_ticks(const std::string& filename);
    void run(csot::Strategy& strategy);

private:
    std::vector<StoredTick> ticks_;   // flat, contiguous — no unique_ptr indirection
    csot::LatencyHistogram hist_;
};
