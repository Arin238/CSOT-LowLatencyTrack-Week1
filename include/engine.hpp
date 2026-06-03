#pragma once

#include <string>
#include <vector>
#include <memory>
#include "histogram.hpp"
#include "strategy.hpp"

namespace csot { struct Tick; }

// Internal storage for ticks: we must own the symbol string data so that
// `std::string_view` fields in `csot::Tick` remain valid while the engine
// replays ticks. We allocate each stored tick on the heap so addresses remain
// stable across vector growth.
struct StoredTick {
    csot::Tick tick;
    std::string backing;
};

class Engine {
public:
    void load_ticks(const std::string& filename);
    void run(csot::Strategy& strategy);

private:
    std::vector<std::unique_ptr<StoredTick>> ticks;
    csot::LatencyHistogram hist_;
};
