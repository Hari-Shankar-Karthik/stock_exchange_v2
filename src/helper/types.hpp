#pragma once

#include "core/constants.hpp"
#include "core/order.hpp"
#include "core/trade_event.hpp"
#include "utils/log.hpp"
#include "utils/spsc_queue.hpp"

namespace exchange::helper
{
    using core::AlignedOrder;
    using core::EVENT_BUFFER_SIZE;
    using core::INCOMING_BUFFER_SIZE;
    using core::REPLAY_LOG_PAGES;
    using core::TradeEvent;
    using utils::Log;
    using utils::SPSCQueue;

    using OrderQueue = SPSCQueue<AlignedOrder, INCOMING_BUFFER_SIZE>;
    using TradeQueue = SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE>; // TODO: Align TradeEvent
    using ReplayLog = Log<AlignedOrder, REPLAY_LOG_PAGES>;
}
