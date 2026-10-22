# 2 小时补课：从 socket 到 epoll

> 起点：没写过 socket，没听过 epoll。
> 目标：跑通三个实验后，能用自己的话讲清"一个线程怎么同时服务一千个连接"。
> 语言用 C（不是 C++），因为 epoll 本身就是 C API，你现在的 C 功底够用。

---

## 时间表

| 时间 | 内容 | 产出 |
|---|---|---|
| 0:00–0:10 | 概念：fd、socket、TCP、端口 | 能解释"连接"到底是什么 |
| 0:10–0:30 | 实验 1：用 Python 服务器 + `nc` 观察四个系统调用 | 看到 accept/read/write |
| 0:30–0:55 | 实验 2：阻塞 echo 服务器（单连接） | 亲手体验"第二个连接被卡住" |
| 0:55–1:10 | 概念：多进程 / 多线程 / 多路复用 | 能说出三种方案各自的代价 |
| 1:10–1:30 | 概念：epoll 三件套 + LT/ET | 能画出"关注列表 / 就绪列表" |
| 1:30–1:50 | 实验 3：epoll echo，两个连接同时通 | 亲眼看到单线程并发 |
| 1:50–2:00 | 10 题小测 | 错题对应回上面某节 |

---

## 0:00–0:10 概念：一切从"文件描述符"开始

Linux 里**打开的东西都是 fd**（file descriptor），就是一个整数：

```bash
ls /proc/self/fd        # 0=stdin, 1=stdout, 2=stderr
```

网络连接也是 fd。一个 TCP 服务器要做的四件事：

```
socket()   → 创建一个"电话机"（fd）
bind()     → 绑定端口（电话号码）
listen()   → 开始接电话
accept()   → 接起一通电话，返回一个新的 fd（这条连接）
read/write → 在这个新 fd 上收发数据
```

客户端对应的是 `socket()` → `connect()`。

**关键**：`accept` 返回的是**新 fd**，代表“某一条连接”；`listen` 的那个 fd 只负责“接电话”，不传数据。

> 检查点：`listen_fd` 和 `accept` 返回的 `client_fd`，区别是什么？

---

## 0:10–0:30 实验 1：观察一次真实连接

终端 A：

```bash
python3 - <<'EOF'
import socket
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", 9001))
s.listen()
print("listening on 9001")
c, addr = s.accept()
print("accepted", addr)
while True:
    data = c.recv(1024)
    if not data:
        break
    c.sendall(data)
EOF
```

终端 B：

```bash
nc 127.0.0.1 9001
# 输入 hello 回车
```

终端 C（观察系统调用）：

```bash
strace -f -e trace=network,read,write -p $(pgrep -f "python3 -" | head -1)
```

你会看到 `accept`、`read`、`write` 依次出现。

> 检查点：为什么 `nc` 一连接，服务器就打印了 "accepted"？

---

## 0:30–0:55 实验 2：阻塞服务器，体验"卡住"

代码：`labs/socket_echo_blocking.c`

```bash
cd ~/json-tutorial/aegis-gateway
gcc -O2 -Wall -Wextra labs/socket_echo_blocking.c -o /tmp/blk
/tmp/blk 9101
```

然后开**两个**终端：

```bash
# 终端 1
nc 127.0.0.1 9101
# 终端 2
nc 127.0.0.1 9101
```

现象：终端 2 连上了（TCP 三次握手由内核完成），但**收不到任何回应**，直到终端 1 断开。

原因：

```c
int fd = accept(...);       /* 只在循环开头 accept 一次 */
while ((n = read(fd, ...)) > 0) {   /* 一直卡在这个连接上 */
    write(fd, buf, n);
}
close(fd);
/* 第一个连接结束，才会回到循环顶部 accept 第二个 */
```

`read()` 在没数据时会**阻塞**，这个线程就钉死在第一条连接上。

> 检查点：如果 1000 个客户端同时连，这个服务器能服务几个？

---

## 0:55–1:10 概念：三种并发方案

| 方案 | 做法 | 代价 |
|---|---|---|
| 多进程 | `fork` 一个子进程处理每个连接 | 进程重，内存大，切换贵 |
| 多线程 | 一个线程处理一个连接 | 线程比进程轻，但 1000 线程仍很重；共享状态要加锁 |
| **多路复用** | 一个线程用 `epoll` 等所有 fd，谁有数据处理谁 | 需要把逻辑拆成“事件 + 回调”，代码复杂一点 |

`select` / `poll` 也是多路复用，但每次调用都要把“关注哪些 fd”重新传给内核，内核每次重新扫描——连接多了就是 O(n)。

`epoll` 的改进：**登记一次，内核维护就绪列表，只返回有事件的 fd**。

---

## 1:10–1:30 概念：epoll 三件套

```c
int ep = epoll_create1(0);                  /* 1. 建一个 epoll 实例 */

struct epoll_event ev;
ev.events = EPOLLIN;                        /* 关心“可读” */
ev.data.fd = fd;
epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);      /* 2. 登记一个 fd */

struct epoll_event events[64];
int n = epoll_wait(ep, events, 64, -1);     /* 3. 等，返回就绪的 fd 个数 */
```

内核里有两份结构：

```
关注列表：fd 5, 7, 9 ...
就绪列表：fd 7（有数据）
epoll_wait 返回时，只把就绪列表拷给你
```

**LT（水平触发，默认）**：缓冲区还有数据就一直通知。
**ET（边缘触发）**：只在状态变化时通知一次，必须一次读到 `EAGAIN`。

> 检查点：如果 1000 个连接里只有 2 个有数据，`epoll_wait` 返回几个？

---

## 1:30–1:50 实验 3：epoll echo

代码：`labs/epoll_echo.c`

```bash
gcc -O2 -Wall -Wextra labs/epoll_echo.c -o /tmp/ep
/tmp/ep 9102
```

再开两个 `nc 127.0.0.1 9102`，**两个都能立刻回显**——同一个线程，同时服务两条连接。

用 `strace` 观察：

```bash
strace -f -e trace=epoll_wait,accept4,read,write /tmp/ep 9102
```

你会看到 `epoll_wait` 返回 1 或 2，然后处理对应的 fd。

### 代码里三个关键点

1. **监听 fd 也注册进 epoll**：`accept` 本身也是“可读事件”；
2. **socket 设成非阻塞**：`accept`/`read` 不能卡住事件循环；
3. **每次事件把数据读完**（循环到 `EAGAIN`）：LT 模式下这是好习惯，ET 模式下是必须。

> 检查点：为什么 `listen_fd` 也要 `epoll_ctl(ADD)`？

---

## 1:50–2:00 十题小测

1. `socket()` 返回什么？它代表什么？
2. `listen` 的 fd 和 `accept` 的 fd 有什么区别？
3. 阻塞 `read` 没数据时会怎样？
4. 为什么“一连接一线程”撑不住 1000 连接？
5. `select` 和 `epoll` 最大的区别是什么？
6. `epoll_create1` / `epoll_ctl` / `epoll_wait` 各自干什么？
7. `epoll_wait` 返回的数字是什么含义？
8. LT 和 ET 的区别？哪个更容易写错？
9. 为什么 epoll 里的 socket 要设非阻塞？
10. Asio 的 `io.run()` 和 `epoll_wait` 是什么关系？

### 答案

1. 一个文件描述符，代表一个 socket 对象。
2. listen fd 只负责接连接；accept fd 代表一条具体连接，用来读写。
3. 线程阻塞，直到有数据、连接关闭或出错。
4. 线程有栈和内核调度开销，1000 线程内存和切换成本很高。
5. select 每次要重传 fd 集合并 O(n) 扫描；epoll 登记一次、只返回就绪 fd。
6. 创建实例 / 增删改关注列表 / 等待事件。
7. 就绪的事件个数（不是 fd 总数）。
8. LT 有数据就反复通知，简单；ET 只在变化时通知一次，必须读到 EAGAIN，容易写错。
9. 避免 accept/read 卡住整个事件循环。
10. `io.run()` 内部就是 epoll_wait 循环 + 回调分发。

---

## 完成标志

- [ ] 实验 2 亲眼看到第二个连接被卡住
- [ ] 实验 3 两个连接同时回显
- [ ] 十题答对 8 题以上
- [ ] 能用一句话解释“一个线程怎么服务一千个连接”
