#include "aegis/net/echo_server.hpp"

#include <array>
#include <memory>
#include <utility>

namespace aegis {
namespace {

using boost::asio::ip::tcp;

/// 一条连接。对应 epoll 版本里 accept 出来的 client_fd。
class EchoSession : public std::enable_shared_from_this<EchoSession> {
public:
    explicit EchoSession(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() { do_read(); }

private:
    void do_read() {
        // 关键：捕获 self，让“未完成的异步操作”持有对象，
        // 防止回调执行前 session 被销毁。
        auto self = shared_from_this();
        socket_.async_read_some(
            boost::asio::buffer(buffer_),
            [this, self](const boost::system::error_code& ec, std::size_t n) {
                if (ec) {
                    return;  // 对端关闭或出错
                }
                do_write(n);
            });
    }

    void do_write(std::size_t n) {
        auto self = shared_from_this();
        // async_write 内部循环直到写完，对应 C 版本里的 write 循环。
        boost::asio::async_write(
            socket_, boost::asio::buffer(buffer_, n),
            [this, self](const boost::system::error_code& ec, std::size_t) {
                if (ec) {
                    return;
                }
                do_read();  // 回显完继续读
            });
    }

    tcp::socket socket_;
    std::array<char, 4096> buffer_{};
};

}  // namespace

EchoServer::EchoServer(boost::asio::io_context& io, unsigned short port)
    : io_(io), acceptor_(io) {
    tcp::endpoint endpoint(tcp::v4(), port);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
}

void EchoServer::start() { do_accept(); }

void EchoServer::do_accept() {
    // 每次 accept 完都再挂一个，对应 C 版本里 for(;;) accept 的“继续接”。
    acceptor_.async_accept(
        [this](const boost::system::error_code& ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<EchoSession>(std::move(socket))->start();
            }
            do_accept();
        });
}

}  // namespace aegis
