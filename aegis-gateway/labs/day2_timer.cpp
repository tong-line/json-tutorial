#include <boost/asio.hpp>

#include <chrono>
#include <cstdio>

int main() {
    boost::asio::io_context io;
    boost::asio::steady_timer timer(io, std::chrono::seconds(1));

    timer.async_wait([](const boost::system::error_code& ec) {
        std::printf("回调执行：1 秒到了（ec=%s）\n", ec ? ec.message().c_str() : "ok");
    });

    std::puts("main：注册完回调，进入 io.run()，这里会阻塞");
    io.run();
    std::puts("main：io.run() 返回，程序退出");
    return 0;
}
