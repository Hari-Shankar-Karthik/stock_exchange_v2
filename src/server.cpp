#include <thread>

#include "config/config.hpp"
#include "core/constants.hpp"
#include "core/order.hpp"
#include "core/trade_event.hpp"
#include "market_data/publisher.hpp"
#include "matching/engine.hpp"
#include "session/session_manager.hpp"
#include "utils/spsc_queue.hpp"

using exchange::config::SERVER_IP_ADDRESS;
using exchange::config::SERVER_PORT;
using exchange::core::EVENT_BUFFER_SIZE;
using exchange::core::INCOMING_BUFFER_SIZE;
using exchange::core::LOG_BUFFER_SIZE;
using exchange::core::Order;
using exchange::core::TradeEvent;
using exchange::market_data::MarketDataPublisher;
using exchange::session::SessionManager;
using exchange::utils::SPSCQueue;

SPSCQueue<Order, INCOMING_BUFFER_SIZE> incomingQueue;
SPSCQueue<Order, LOG_BUFFER_SIZE> loggingQueue;
SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> tradeEventQueue, marketDataQueue;

SessionManager sessionManager(incomingQueue, tradeEventQueue, SERVER_IP_ADDRESS, SERVER_PORT);

void exchangeFunc()
{
    exchange::matching::MatchingEngine me(incomingQueue, loggingQueue, tradeEventQueue, marketDataQueue);
    std::cerr << "[ENGINE] startup complete" << std::endl;
    me.run();
}

void publisherFunc()
{
    exchange::market_data::MarketDataPublisher publisher(marketDataQueue);
    publisher.run();
}

int main()
{
    std::thread sender(&SessionManager::sender, &sessionManager);
    std::thread receiver(&SessionManager::receiver, &sessionManager);
    std::thread exchange(exchangeFunc);
    std::thread publisher(publisherFunc);

    sender.join();
    exchange.join();
    receiver.join();
}