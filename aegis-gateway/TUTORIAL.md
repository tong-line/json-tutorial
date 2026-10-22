# Day 2 实验课：从定时器到异步 echo 服务器

> 目标：不背概念，靠 5 个小实验亲手感受 `io_context`、回调、对象生命周期。
> 每个实验都能单独跑，跑完再进下一个。全程约 3–4 小时。

---

## 0. 先搞懂三件事

### 0.1 这个项目要做什么

想象你公司有三个后端服务：

- 用户服务：`127.0.0.1:9001`
- 订单服务：`127.0.0.1:9002`
- 一个 AI 工具服务器（MCP）：由子进程提供，能读文件、查 git

前端不想记三个地址，只想访问 `127.0.0.1:8080`。于是你写一个**网关**：

```
浏览器  ──/api/users/1──►  网关 :8080  ──转发──►  用户服务 :9001
浏览器  ──/api/orders───►  网关 :8080  ──转发──►  订单服务 :9002
AI 客户端 ──tools/call──►  网关 :8080  ──转发──►  MCP 子进程
```

网关要负责：**按路径选后端、超时、后端挂了返回 502、限流、记日志和指标**。

本项目就是把这件事做完整，包括测试和压测数据。

### 0.2 Asio 是什么

**Asio 是一个 C++ 的异步 I/O 库**（Boost 的一部分），把 Linux 的 epoll / macOS 的 kqueue 封装成“事件循环 + 回调”。

对比三种服务器写法：

| 写法 | 代码样子 | 问题 |
|---|---|---|
| 阻塞 | `read()` 卡住等数据 | 一个连接一个线程，1000 连接 = 1000 线程 |
| 手写 epoll | 自己维护 fd 状态机 | 难写、易错、难测 |
| **Asio** | `async_read(..., 回调)` | 代码像顺序的，执行是事件驱动的 |

餐厅类比：

- 阻塞 = 服务员站在你桌边等你吃完；
- 异步 = 点完单给你号码牌，厨房好了叫你，服务员去服务别人。

`io_context` 就是那个“叫号系统”，`run()` 是服务员开始工作。

### 0.3 你会写出什么

最终是一个可执行文件 `aegis`：

```bash
./aegis 8080          # 监听 8080
curl localhost:8080/api/users/1   # 被转发到 9001
```

今天只做第一步：**echo 服务器**（收到什么回什么），把 Asio 的骨架跑通。

---

## Lab 1：定时器（15 分钟）

不碰网络，只看“注册回调 → 事件循环 → 回调执行”。

代码：`labs/day2_timer.cpp`

```cpp
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
```

编译运行：

```bash
cd ~/json-tutorial/aegis-gateway
g++ -std=c++20 -Wall -Wextra labs/day2_timer.cpp -o /tmp/day2_timer -lboost_system
/tmp/day2_timer
```

预期输出（注意顺序）：

```
main：注册完回调，进入 io.run()，这里会阻塞
（等 1 秒）
回调执行：1 秒到了（ec=ok）
main：io.run() 返回，程序退出
```

**要理解的三点**：

1. `async_wait` 只是“登记”，立刻返回，没有等待；
2. `io.run()` 才开始等事件，没事件就阻塞；
3. 回调由 `run()` 在**同一个线程**里调用。

---

## Lab 2：最原始的 socket（可选，10 分钟）

用 Python 起一个阻塞式 echo 服务器，感受“一连接一线程”的写法：

```bash
python3 - <<'EOF'
import socket, threading
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", 7000)); s.listen()
def handle(c):
    while True:
        data = c.recv(1024)
        if not data: break
        c.sendall(data)
    c.close()
while True:
    c, _ = s.accept()
    threading.Thread(target=handle, args=(c,), daemon=True).start()
EOF
```

另开终端：

```bash
printf 'hello\n' | nc 127.0.0.1 7000
```

这是“每连接一线程”。我们的目标是用**一个线程**处理所有连接——这就是 Asio 要做的事。

---

## Lab 3：异步 echo 服务器（核心，1–2 小时）

### 3.1 加文件

- `include/aegis/net/echo_server.hpp`
- `src/net/echo_server.cpp`

内容见项目里对应文件（或直接照抄 Day 2 讲解里的代码）。

### 3.2 改 CMake

在 `CMakeLists.txt` 的 `add_library(aegis_core ...)` 里加一行：

```cmake
    src/net/echo_server.cpp
```

### 3.3 改 main.cpp

```cpp
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <boost/asio.hpp>

#include "aegis/core/version.hpp"
#include "aegis/net/echo_server.hpp"

int main(int argc, char** argv) {
    unsigned short port = 8080;
    if (argc > 1) port = static_cast<unsigned short>(std::atoi(argv[1]));

    try {
        boost::asio::io_context io;
        aegis::EchoServer server(io, port);
        server.start();
        std::printf("aegis-gateway %.*s listening on 0.0.0.0:%u\n",
                    static_cast<int>(aegis::version().size()),
                    aegis::version().data(), port);
        io.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
```

### 3.4 构建并测试

```bash
cmake --build build -j
./build/aegis 8080
```

另开一个终端：

```bash
nc 127.0.0.1 8080
# 输入 hello 回车 → 原样返回
```

或用一行命令：

```bash
printf 'hello\n' | nc -w1 127.0.0.1 8080
```

预期：返回 `hello`。

### 3.5 三个关键代码点

1. `auto self = shared_from_this();` + 捕获 `self`
   否则 session 可能在回调前析构，回调里的 `this` 悬空 → 段错误。
2. `async_write` 而不是 `async_write_some`
   `write_some` 可能只写一半；`async_write` 内部循环写完。
3. `do_accept()` 递归
   `async_accept` 一次只管一个连接，处理完要再挂一个。

---

## Lab 4：观察对象生命周期（30 分钟）

在 `EchoSession` 里加：

```cpp
~EchoSession() { std::puts("[session] 销毁"); }
```

打开两个终端分别 `nc 127.0.0.1 8080`：

- 连接建立：构造；
- 断开：`~EchoSession` 打印“销毁”。

再加编译开关看细节：

```bash
cmake -S . -B build-trace -DCMAKE_CXX_FLAGS="-DBOOST_ASIO_ENABLE_HANDLER_TRACKING"
cmake --build build-trace -j
./build-trace/aegis 8080
```

Asio 会打印每个 handler 的创建/执行/销毁，你会看到“为什么对象迟迟不销毁”（因为有未完成的 handler 持有 `self`）。

---

## Lab 5：多线程与 strand（30 分钟，理解即可）

把 `io.run()` 换成两个线程：

```cpp
std::thread t1([&io]{ io.run(); });
std::thread t2([&io]{ io.run(); });
t1.join(); t2.join();
```

问题：同一个 session 的两个回调**可能同时**在两个线程执行。解决方案是 `strand`：

```cpp
boost::asio::strand<boost::asio::io_context::executor_type> strand_{
    boost::asio::make_strand(io)};
// 之后所有 async_read/async_write 用 boost::asio::bind_executor(strand_, handler)
```

今天只需要知道：**多线程 + 同一对象 = 要串行化**。

---

## 常见报错

| 报错 | 原因 | 解决 |
|---|---|---|
| `Address already in use` | 上次进程没退干净 | `pkill aegis`，并保留 `reuse_address(true)` |
| `Permission denied` | 端口 < 1024 | 换 8080 |
| `nc` 没输出 | 发送后没关写端 | 用 `nc -w1` 或 `printf ... \| nc` |
| 段错误 / UAF | 回调没持有 `self` | 检查 `shared_from_this` |

---

## 完成标志

- [ ] Lab 1 看到正确输出顺序
- [ ] Lab 3 `nc` 能回显
- [ ] Lab 4 能看到 session 析构打印
- [ ] 能回答：为什么捕获 `self`？为什么 `async_write`？
- [ ] `git commit -m "W1 D2: 异步 echo server"`
