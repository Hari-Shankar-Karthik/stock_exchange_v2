#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

#include "core/order.hpp"
#include "core/types.hpp"

namespace exchange::test
{
    // Defined outside OrderGenerator: a nested struct with default member initializers can't be
    // used in the enclosing class's default arguments (GCC rejects it).
    struct GeneratorConfig
    {
        core::Price minPrice = 95; // narrow band so orders keep crossing each other
        core::Price maxPrice = 105;
        core::Quantity maxQuantity = 10;
        core::ClientId numClients = 4;
        unsigned marketPercent = 10;
        unsigned iocPercent = 10;
        // Engine slabs hold 1024 resting orders and overflowing is UB. Above this, only send market orders to drain.
        std::size_t softRestingLimit = 400;
    };

    class OrderGenerator
    {
    public:
        explicit OrderGenerator(std::uint64_t seed, GeneratorConfig config = {}) : rng(seed), cfg(config) {}

        core::Order next()
        {
            core::Order o{};
            o.side = below(2) == 0 ? core::Side::BUY : core::Side::SELL;
            o.clientId = 1 + below(cfg.numClients);
            o.quantity = static_cast<core::Quantity>(1 + below(cfg.maxQuantity));
            o.sequenceNumber = ++issued;

            const std::uint64_t roll = below(100);
            if (roll < cfg.marketPercent)
            {
                o.orderType = core::OrderType::MARKET;
                o.price = 0; // ignored for market orders
            }
            else if (roll < cfg.marketPercent + cfg.iocPercent)
            {
                o.orderType = core::OrderType::IOC;
                o.price = randomPrice();
            }
            else
            {
                o.orderType = core::OrderType::LIMIT;
                o.price = randomPrice();
            }
            return o;
        }

    private:
        // rng() % n rather than std::uniform_int_distribution: the standard doesn't specify the
        // distribution's algorithm, so the same seed could give different streams on libstdc++ vs libc++.
        // The tiny modulo bias is irrelevant here.
        std::uint64_t below(std::uint64_t n) { return rng() % n; }

        core::Price randomPrice()
        {
            const auto range = static_cast<std::uint64_t>(cfg.maxPrice - cfg.minPrice) + 1;
            return cfg.minPrice + static_cast<core::Price>(below(range));
        }

        std::mt19937_64 rng;
        GeneratorConfig cfg;
        core::SequenceNumber issued{0};
    };
}