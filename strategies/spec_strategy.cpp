#include <cmath>
#include <array>
#include <string_view>
#include <vector>

#include "../include/strategy.hpp"

namespace {

struct SymbolState {
    double sum = 0.0;
    double sum_sq = 0.0;
    uint32_t count = 0;
    uint32_t head = 0;
    int32_t position = 0;
    std::array<double, 64> mids{};
};

static inline __attribute__((always_inline))
bool process_tick(SymbolState& st, const csot::Tick& t, csot::Order& order) {
    const double mid = (t.bid_px + t.ask_px) * 0.5;

    if (st.count == 64) {
        const double old = st.mids[st.head];
        st.sum -= old;
        st.sum_sq -= old * old;
    }

    st.mids[st.head] = mid;
    st.sum += mid;
    st.sum_sq += mid * mid;

    st.head = (st.head + 1) & 63;
    if (st.count < 64) ++st.count;

    if (__builtin_expect(st.count < 64, 0)) return false;

    const double mean = st.sum * 0.015625; // 1.0 / 64.0
    const double variance = (st.sum_sq * 0.015625) - (mean * mean);

    if (__builtin_expect(variance < 1e-18, 0)) return false; // stddev < 1e-9

    const double diff = mid - mean;
    const double diff_sq = diff * diff;

    if (st.position == 0) {
        if (__builtin_expect(diff_sq >= 4.0 * variance, 0)) {
            if (diff > 0) {
                order = csot::Order{csot::Order::Side::SELL, t.symbol, t.bid_px, 1};
                return true;
            } else {
                order = csot::Order{csot::Order::Side::BUY, t.symbol, t.ask_px, 1};
                return true;
            }
        }
        return false;
    }

    if (__builtin_expect(diff_sq <= 0.25 * variance, 0)) {
        if (st.position > 0) {
            order = csot::Order{csot::Order::Side::SELL, t.symbol, t.bid_px, static_cast<uint32_t>(st.position)};
            return true;
        } else {
            order = csot::Order{csot::Order::Side::BUY, t.symbol, t.ask_px, static_cast<uint32_t>(-st.position)};
            return true;
        }
    }

    return false;
}

class SpecStrategy : public csot::Strategy {
public:
    void on_init() override {
        symbol_count_ = 0;
    }

    std::vector<csot::Order> on_tick(const csot::Tick& t) override {
        std::string_view sv = t.symbol;
        uint32_t id = 0xFFFFFFFF;
        for (uint32_t i = 0; i < symbol_count_; ++i) {
            if (symbols_[i] == sv) {
                id = i;
                break;
            }
        }

        if (__builtin_expect(id == 0xFFFFFFFF, 0)) {
            if (symbol_count_ == 64) return {};
            id = symbol_count_++;
            symbols_[id] = sv;
        }

        auto &st = states_[id];
        csot::Order order;
        if (process_tick(st, t, order)) {
            return {order};
        }

        return {};
    }

    void on_fill(const csot::Order& o, double fill_price, uint32_t fill_qty) override {
        (void)fill_price;
        std::string_view sv = o.symbol;
        for (uint32_t i = 0; i < symbol_count_; ++i) {
            if (symbols_[i] == sv) {
                auto &st = states_[i];
                if (o.side == csot::Order::Side::BUY) {
                    st.position += static_cast<int32_t>(fill_qty);
                } else {
                    st.position -= static_cast<int32_t>(fill_qty);
                }
                return;
            }
        }
    }

private:
    std::array<std::string_view, 64> symbols_{};
    std::array<SymbolState, 64> states_{};
    uint32_t symbol_count_ = 0;
};

} // anonymous namespace

extern "C" csot::Strategy* create_strategy() {
    return new SpecStrategy();
}
