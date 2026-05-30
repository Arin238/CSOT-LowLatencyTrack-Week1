#include "../include/strategy.hpp"
#include <vector>

namespace{

class NullStrategy : public csot::Strategy {
public:
    std::vector<csot::Order> on_tick(const csot::Tick& t) override {
        return {};
    }
    void on_fill(const csot::Order& o,
                 double        fill_price,
                 uint32_t      fill_qty) override {}
};

}

extern "C" csot::Strategy* create_strategy() {
    return new NullStrategy();
}