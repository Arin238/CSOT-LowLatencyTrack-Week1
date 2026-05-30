#include <cmath>
#include <array>
#include <unordered_map>
#include <string>
#include <vector>

#include "../include/strategy.hpp"

namespace {

struct SymbolState {
    std::array<double, 64> mids{};
    uint32_t count = 0;
    uint32_t head = 0;
    int32_t position = 0; // -1,0,+1
};

class SpecStrategy : public csot::Strategy {
public:
    void on_init() override {
        states_.reserve(128);
    }

    std::vector<csot::Order> on_tick(const csot::Tick& t) override {
        const double mid = (t.bid_px + t.ask_px) * 0.5;

        std::string key(t.symbol);
        auto &st = states_[key];

        st.mids[st.head] = mid;
        st.head = (st.head + 1) & 63;
        if (st.count < 64) ++st.count;

        if (st.count < 64) return {};

        double sum = 0.0;
        for (double x : st.mids) sum += x;
        const double mean = sum / 64.0;

        double sq = 0.0;
        for (double x : st.mids) {
            const double d = x - mean;
            sq += d * d;
        }
        const double variance = sq / 64.0;
        const double stddev = std::sqrt(variance);

        if (stddev < 1e-9) return {};

        const double z = (mid - mean) / stddev;
        const double abs_z = std::abs(z);

        if (st.position == 0) {
            if (z >= 2.0) {
                return {csot::Order{csot::Order::Side::SELL, t.symbol, t.bid_px, 1}};
            }
            if (z <= -2.0) {
                return {csot::Order{csot::Order::Side::BUY, t.symbol, t.ask_px, 1}};
            }
            return {};
        }

        if (st.position > 0 && abs_z <= 0.5) {
            return {csot::Order{csot::Order::Side::SELL, t.symbol, t.bid_px, static_cast<uint32_t>(st.position)}};
        }

        if (st.position < 0 && abs_z <= 0.5) {
            return {csot::Order{csot::Order::Side::BUY, t.symbol, t.ask_px, static_cast<uint32_t>(-st.position)}};
        }

        return {};
    }

    void on_fill(const csot::Order& o, double fill_price, uint32_t fill_qty) override {
        (void)fill_price;
        std::string key(o.symbol);
        auto it = states_.find(key);
        if (it == states_.end()) return;
        if (o.side == csot::Order::Side::BUY) {
            it->second.position += static_cast<int32_t>(fill_qty);
        } else {
            it->second.position -= static_cast<int32_t>(fill_qty);
        }
    }

private:
    std::unordered_map<std::string, SymbolState> states_;
};

} // anonymous namespace

extern "C" csot::Strategy* create_strategy() {
    return new SpecStrategy();
}
