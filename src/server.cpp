#include <iostream> // TODO: TEMPORARY
#include <thread>

#include "core/constants.hpp"
#include "core/order.hpp"
#include "core/trade_event.hpp"
#include "matching/engine.hpp"
#include "session/session_manager.hpp"
#include "utils/spsc_queue.hpp"

using exchange::core::EVENT_BUFFER_SIZE;
using exchange::core::INCOMING_BUFFER_SIZE;
using exchange::core::LOG_BUFFER_SIZE;
using exchange::core::Order;
using exchange::core::SERVER_IP_ADDRESS;
using exchange::core::SERVER_PORT;
using exchange::core::TradeEvent;
using exchange::session::SessionManager;
using exchange::utils::SPSCQueue;

SPSCQueue<Order, INCOMING_BUFFER_SIZE> incomingQueue;
SPSCQueue<Order, LOG_BUFFER_SIZE> loggingQueue;
// TODO: We need 2 event queues, one for fill report and one for market data
SPSCQueue<TradeEvent, EVENT_BUFFER_SIZE> tradeEventQueue;

SessionManager sessionManager(incomingQueue, tradeEventQueue, SERVER_IP_ADDRESS, SERVER_PORT);

void exchangeFunc()
{
    exchange::matching::MatchingEngine me(incomingQueue, loggingQueue, tradeEventQueue);
    std::cerr << "[ENGINE] startup complete" << std::endl;
    me.run();
}

int main()
{
    std::thread sender(&SessionManager::sender, &sessionManager), exchange(exchangeFunc), receiver(&SessionManager::receiver, &sessionManager);

    sender.join();
    exchange.join();
    receiver.join();
}