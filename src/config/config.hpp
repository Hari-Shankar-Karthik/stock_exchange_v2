#pragma once

#include <cstdint>
#include <string_view>

namespace exchange::config
{
    constexpr std::string_view SERVER_IP_ADDRESS = "127.0.0.1";
    constexpr std::uint16_t SERVER_PORT = 6969;

    constexpr std::string_view MULTICAST_IP_ADDR = "239.0.0.1";
    constexpr std::uint16_t MULTICAST_PORT = 9696;

    constexpr std::string_view REPLAY_LOG_DIR = "/home/theta/Hari/coding/stock_exchange_v2/replay_log";
}