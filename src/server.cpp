#include <chrono>
#include <thread>

#include "helper/types.hpp"
#include "market_data/publisher.hpp"
#include "matching/engine.hpp"
#include "session/session_manager.hpp"

using namespace exchange::helper;
using exchange::config::SERVER_IP_ADDRESS;
using exchange::config::SERVER_PORT;
using exchange::session::SessionManager;

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