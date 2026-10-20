# 高并发服务器（muduo 风格）30 天作战计划

> 目标：一个能写进简历、能讲一小时的 epoll 高并发服务器。
> 风格：muduo（one loop per thread），不是照抄，是自己按设计重写。

## 0. 环境：macOS 写不了 epoll，先上 Linux

- 首选 **OrbStack**（个人使用免费；若收费就换 Multipass）装 Ubuntu 24.04 ARM 虚拟机
- 备选：Multipass（完全免费）、GitHub Codespaces（每月免费额度）
- 装好后：`sudo apt install build-essential cmake g++ git gdb apache2-utils`
- 压测工具：`ab`（apache2-utils 自带），`wrk` 从源码编译
- 验收：能在 VM 里编译运行用 epoll/timerfd/eventfd 的代码

## 1. 技术栈决定（开工前定死）

- C++17 + CMake，只依赖 Linux 系统调用，不引 boost
- 开发期 `-Wall -Wextra` + ASan/UBSan
- 延续 json-tutorial 的 TDD 习惯：Buffer、HTTP 解析器写单元测试

## 2. 里程碑（每格 1 天，可被打断）

### W2：核心事件循环（裸 epoll → 抽象）

- [ ] D10 环境搭建 + v0.1：裸 epoll 单线程 echo（不写任何抽象）
- [ ] D11-12 v0.2：EventLoop + Channel + EpollPoller + Acceptor + TcpConnection
- [ ] D13-14 Buffer（前置空间、粘包处理）+ 写缓冲与半关闭
- [ ] D15-16 TcpServer + 回调 + shared_ptr 生命周期（handleClose 放循环末尾）

### W3：多线程 + 完整功能

- [ ] D17-18 EventLoopThread + EventLoopThreadPool（one loop per thread）
- [ ] D19-20 TimerQueue（timerfd）+ runInLoop/queueInLoop + eventfd 唤醒
- [ ] D21-23 HTTP 解析器 + 服务器 + keep-alive；ab/wrk 第一轮压测；忽略 SIGPIPE、优雅关闭

### W4：收尾 + 数据 + 博客

- [ ] D24-25 异步日志（双缓冲简化版）
- [ ] D26-27 压测调优：backlog、TCP_NODELAY、线程数对比，整理数据表
- [ ] D28-30 README + 博客 + 简历更新 v3 + 重新投递

## 3. 每个里程碑对应的面试问题（写完就记进 notes.md）

- [ ] 为什么用 epoll 的 LT 模式而不是 ET？muduo 的理由是什么？
- [ ] one loop per thread 为什么高效？什么场景会失效？
- [ ] TcpConnection 为什么要 shared_ptr + enable_shared_from_this？
- [ ] 为什么定时器用 timerfd 而不是 setitimer？
- [ ] Buffer 为什么要有前置空间？粘包/半包怎么处理？
- [ ] 跨线程操作连接为什么必须 runInLoop？

## 4. 必踩的坑（提前知道）

1. TcpConnection 析构时机：Channel 持 weak_ptr，handleClose 排到循环末尾再销毁
2. SIGPIPE 必须忽略，否则 send 到已关闭的连接直接杀进程
3. LT 模式每次回调要读到 EAGAIN 为止
4. 跨线程只能 runInLoop/queueInLoop，不能直接碰 fd 和 Channel

## 5. 参考资料

- 陈硕《Linux 多线程服务端编程：使用 muduo C++ 网络库》第 6-8 章
- muduo 源码 `net/` 目录（EventLoop / Channel / TcpConnection / Buffer）
- 陈硕知乎专栏：one loop per thread、多线程服务器的适用场合

## 6. 验收标准（= 简历可写的内容）

- [ ] echo / HTTP 服务器可运行，ab/wrk 压测有数字（QPS、并发连接）
- [ ] 万级长连接不崩
- [ ] 单元测试 + ASan 全绿
- [ ] 能不看代码讲一小时设计取舍
