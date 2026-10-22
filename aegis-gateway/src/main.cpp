#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <boost/asio.hpp>

#include "aegis/core/version.hpp"
#include "aegis/net/echo_server.hpp"

int main(int argc, char** argv) {
    unsigned short port = 8080;
    if (argc > 1) {
        port = static_cast<unsigned short>(std::atoi(argv[1]));
    }

    try {
        boost::asio::io_context io;
        aegis::EchoServer server(io, port);
        server.start();

        std::printf("aegis-gateway %.*s listening on 0.0.0.0:%u\n",
                    static_cast<int>(aegis::version().size()),
                    aegis::version().data(), port);

        // 把 SIGINT / SIGTERM 也变成异步事件：收到信号 → 停止 acceptor → 退出事件循环。
        boost::asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait([&](const boost::system::error_code& ec, int signo) {
            if (ec) {
                return;
            }
            std::printf("\nreceived signal %d, shutting down\n", signo);
            server.stop();
            io.stop();
        });

        io.run();  // 对应 epoll_wait 循环，直到没有待处理操作
        std::puts("shutdown complete");
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
