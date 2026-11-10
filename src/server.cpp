#include <chrono>
#include <thread>

#include "config/config.hpp"
#include "core/constants.hpp"
#include "core/order.hpp"
#include "core/trade_event.hpp"
#include "market_data/publisher.hpp"
#include "matching/engine.hpp"
#include "session/session_manager.hpp"
#include "utils/log.hpp"
#include "utils/spsc_queue.hpp"

using exchange::config::SERVER_IP_ADDRESS;
using exchange::config::SERVER_PORT;
using exchange::core::EVENT_BUFFER_SIZE;
using exchange::core::INCOMING_BUFFER_SIZE;
using exchange::core::Order;
using exchange::core::REPLAY_LOG_PAGES;
using exchange::core::TradeEvent;
using exchange::market_data::MarketDataPublisher;
using exchange::session::SessionManager;
using exchange::utils::Log;
using exchange::utils::SPSCQueue;

SPSCQueue<Order, INCOMING_BUFFER_SIZE> incomingQueue;
Log<Order, REPLAY_LOG_PAGES> replayLog(std::chrono::steady_clock::now().time_since_epoch().count());
SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> tradeEventQueue, marketDataQueue;

SessionManager sessionManager(incomingQueue, tradeEventQueue, SERVER_IP_ADDRESS, SERVER_PORT);

void senderFunc()
{
    sessionManager.sender();
}

void receiverFunc()
{
    sessionManager.receiver();
}

void matcherFunc()
{
    exchange::matching::MatchingEngine me(incomingQueue, replayLog, tradeEventQueue, marketDataQueue);
    std::cerr << "[ENGINE] startup complete" << std::endl;
    me.run();
}

void loggerFunc()
{
    replayLog.drain();
}

void publisherFunc()
{
    exchange::market_data::MarketDataPublisher publisher(marketDataQueue);
    publisher.run();
}

int main()
{
    std::jthread matcher(matcherFunc);
    std::jthread logger(loggerFunc);
    std::jthread publisher(publisherFunc);
    std::jthread sender(senderFunc);
    std::jthread receiver(receiverFunc);
}