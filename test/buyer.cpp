#include <asio.hpp>
#include <iostream>
#include <thread>

#include "config/config.hpp"
#include "src/core/constants.hpp"
#include "src/core/order.hpp"
#include "src/core/trade_event.hpp"

using exchange::config::SERVER_IP_ADDRESS;
using exchange::config::SERVER_PORT;
using exchange::core::Order;
using exchange::core::OrderType;
using exchange::core::Side;
using exchange::core::TradeEvent;

void readFill(asio::ip::tcp::socket &sock)
{
    static TradeEvent e;

    asio::async_read(
        sock,
        asio::buffer(&e, sizeof(e)),
        [&sock](const asio::error_code &ec, std::size_t bytesRead)
        {
            if (!ec)
            {
                std::cout << e << std::endl;
                readFill(sock);
            }
        });
}

void senderFunc(asio::ip::tcp::socket &sock)
{
    // while (true)
    // {
    Order o;
    o.orderType = OrderType::IOC;
    o.price = 1000;
    o.quantity = 100;
    o.side = Side::BUY;

    asio::write(sock, asio::buffer(&o, sizeof(o)));
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
}

void receiverFunc(asio::io_context &ioc, asio::ip::tcp::socket &sock)
{
    readFill(sock);
    ioc.run();
}

int main()
{
    asio::io_context ioc;
    asio::ip::tcp::socket sock(ioc);
    asio::ip::tcp::endpoint serverEndpoint(asio::ip::make_address(SERVER_IP_ADDRESS), SERVER_PORT);
    sock.connect(serverEndpoint);
    std::cerr << "[BUYER] Connected Successfully" << std::endl;

    std::thread sender(senderFunc, std::ref(sock)), receiver(receiverFunc, std::ref(ioc), std::ref(sock));

    sender.join();
    receiver.join();
}