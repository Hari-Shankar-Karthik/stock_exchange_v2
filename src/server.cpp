#include <chrono>
#include <thread>

#include "helper/types.hpp"
#include "market_data/publisher.hpp"
#include "matching/engine.hpp"
#include "session/session_manager.hpp"
#include "sync/sync_variables.hpp"

using namespace exchange::helper;
using exchange::config::SERVER_IP_ADDRESS;
using exchange::config::SERVER_PORT;
using exchange::session::SessionManager;
using exchange::sync::isProcessingClosed;
using exchange::sync::killSwitch;

OrderQueue incomingQueue;
ReplayLog replayLog(std::chrono::steady_clock::now().time_since_epoch().count());
TradeQueue tradeEventQueue, marketDataQueue;

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
    replayLog.drain(isProcessingClosed);
}

void publisherFunc()
{
    exchange::market_data::MarketDataPublisher publisher(marketDataQueue);
    publisher.run();
}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    std::jthread matcher(matcherFunc);
    std::jthread logger(loggerFunc);
    std::jthread publisher(publisherFunc);
    std::jthread sender(senderFunc);
    std::jthread receiver(receiverFunc);

    char c;
    std::cin.get(c);

    killSwitch.store(true, std::memory_order_release);
}