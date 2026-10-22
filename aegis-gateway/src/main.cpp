#include <cstdio>

#include "aegis/core/version.hpp"

int main() {
    std::printf("aegis-gateway %.*s\n",
                static_cast<int>(aegis::version().size()),
                aegis::version().data());
    std::puts("skeleton OK — 下一步：Asio 异步 echo server（W1 D2）");
    return 0;
}
