#pragma once

#include <boost/asio.hpp>

namespace aegis {

/// 异步 echo 服务器：把收到的字节原样写回。
/// 对应 epoll 版本里的 listen_fd + accept 循环。
class EchoServer {
public:
    EchoServer(boost::asio::io_context& io, unsigned short port);
    void start();

private:
    void do_accept();

    boost::asio::io_context& io_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

}  // namespace aegis
