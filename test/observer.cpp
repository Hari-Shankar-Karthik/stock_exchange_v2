#include <asio.hpp>

#include "src/config/config.hpp"
#include "src/core/trade_event.hpp"

using exchange::config::MULTICAST_IP_ADDR;
using exchange::config::MULTICAST_PORT;
using exchange::core::TradeEvent;

int main()
{
    asio::io_context context;
    asio::ip::udp::socket socket(context);
    const auto multicastAddr = asio::ip::make_address(MULTICAST_IP_ADDR);
    const asio::ip::udp::endpoint multicastEndpoint(multicastAddr, MULTICAST_PORT);

    socket.open(multicastEndpoint.protocol());
    socket.set_option(asio::socket_base::reuse_address(true));
    socket.set_option(asio::ip::multicast::join_group(multicastAddr));
    socket.bind(multicastEndpoint);

    TradeEvent e;
    while (true)
    {
        socket.receive(asio::buffer(&e, sizeof(e)));
        std::cout << "Trade Received: " << e << std::endl;
    }
}