# Aegis Gateway — 4 周执行计划

> 定位：C++20 异步 HTTP 反向代理 + MCP 工具网关。
> 时间假设：4 周 × 约 30 小时 ≈ 120 小时。第 4 周末必须开始投递。
> 硬门槛：W2 结束 HTTP 代理必须跑通；不通过就暂停 MCP 与一切高级功能。

---

## 0. 定位与目标岗位

一句话：**一个能路由、能转发、能限流、有测试和压测数据的异步网关，HTTP 与 MCP 共用一套基础设施。**

| 方向 | 匹配度 | 讲法 |
|---|---|---|
| C++ 后端 / 基础架构 | ★★★ | 异步 I/O、连接管理、超时取消、背压 |
| AI Infra | ★★★ | MCP 工具聚合与调用路由，多下游会话管理 |
| 智驾 / 机器人 | ★★ | 异步事件循环、确定性超时、资源生命周期 |
| 工具链 | ★★ | 工程化、测试、故障注入、性能分析 |
| 量化 | ★ | 不作为主打；可讲低延迟路径与队列 |

**不做**：自研 HTTP 解析器、自研 TLS、服务注册中心、K8s 部署、Windows 兼容。

---

## 1. 架构

```
        HTTP Client                      MCP Host (Claude Desktop 等)
             │                                      │
             ▼                                      ▼
   ┌──────────────────┐                  ┌──────────────────────┐
   │  HTTP Adapter    │                  │   MCP Adapter        │
   │  Beast session   │                  │  JSON-RPC / stdio    │
   └────────┬─────────┘                  └──────────┬───────────┘
            │                                       │
            └──────────────┬────────────────────────┘
                           ▼
              ┌──────────────────────────┐
              │      Shared Core         │
              │  Router / Config         │
              │  RequestContext / 超时    │
              │  SPSC 队列 + 背压         │
              │  Metrics / Logger        │
              └────────────┬─────────────┘
                           │
        ┌──────────────────┴──────────────────┐
        ▼                                     ▼
  HTTP Upstreams                        MCP Upstreams
  (两个 mock 后端)                   (stdio 子进程 ×2)
```

### 模块职责

| 模块 | 职责 | 备注 |
|---|---|---|
| `http::Server` | accept、session 生命周期 | `shared_from_this` + strand |
| `http::Proxy` | 向上游发请求、读响应 | Beast 异步 client |
| `core::Router` | method + path 前缀匹配 | 纯函数，可单测 |
| `core::Config` | 路由/上游/超时/队列容量 | TOML，启动时加载 |
| `core::RequestContext` | 请求 ID、截止时间、取消标志 | 只放跨模块必需字段 |
| `core::SpscQueue` | 有界队列 + 背压 | 本项目的无锁亮点 |
| `mcp::Client` | 子进程 + JSON-RPC 会话 | 换行分隔 JSON |
| `mcp::Registry` | 工具聚合、命名冲突 | `server.tool` 命名空间 |
| `core::Metrics` | 计数、延迟直方图 | `/metrics` 文本输出 |

### 线程模型

- 1 个 `io_context`，N 个线程 `run()`；
- 每个 session 用 `strand` 串行化回调；
- MCP 子进程读写走 `asio::posix::stream_descriptor`，不阻塞 I/O 线程；
- 请求进入后按路由投递到有界 SPSC 队列，worker 批量取出执行；队列满 → `429`（背压）。

---

## 2. 四周逐日任务

### W1：骨架与 HTTP 服务

| 日 | 任务 | 验收 |
|---|---|---|
| D1 | 环境与骨架（✅ 已完成）：CMake + Boost 1.83 + GTest，`ctest` 通过 | `cmake -S . -B build && cmake --build build && ctest --test-dir build` |
| D2 | Asio 异步 echo server：accept + session + `shared_from_this` | `nc 127.0.0.1 8080` 能回显 |
| D3 | 优雅关闭：`signal_set` + 停止 accept + 关闭会话 | Ctrl-C 后端口立刻释放 |
| D4 | Beast HTTP server，返回固定 JSON `{"status":"ok"}` | `curl` 能拿到 200 |
| D5 | 两个 mock 后端（Python `http.server` 两端口） | 两个端口各自返回不同内容 |
| D6 | `Router` 纯函数 + 单测（前缀、方法、404） | `ctest` 新增用例通过 |
| D7 | README + CI 绿 + 周复盘 | GitHub Actions 通过 |

**W1 验收**：客户端可连续请求；重启无端口占用；`ctest` 全绿。

### W2：HTTP 反向代理（硬门槛）

| 日 | 任务 | 验收 |
|---|---|---|
| D8 | 配置加载（toml++，单文件依赖）：路由表、上游、超时、队列容量 | 配置错误时启动失败并打印原因 |
| D9 | 路由匹配接入 + 404/502 错误映射 | 未匹配路径返回 404 |
| D10 | Beast 异步 client：resolve → connect → write → read | 能取到 mock 后端响应 |
| D11 | 请求/响应透传（header、body、keep-alive、Content-Length） | `curl -v` 对比直连与经网关一致 |
| D12 | 超时（`steady_timer` + `cancel`）、上游拒绝 → 502 | 慢后端 504、宕机 502 |
| D13 | 集成测试：两后端 + 路由断言 | `ctest` 集成用例通过 |
| D14 | 故障测试：关掉一个后端、半包、超大 header | 网关不崩、错误码正确 |

**W2 验收（硬门槛）**：路由正确；上游宕机不崩；超时不会无限等待；有集成测试。

### W3：MCP 接入

| 日 | 任务 | 验收 |
|---|---|---|
| D15 | 锁定 MCP 规范版本（如 `2025-06-18`），确认 stdio 是**换行分隔 JSON** | 能抓包看到一行一条消息 |
| D16 | 子进程启动 + 管道（`posix::stream_descriptor`，`waitpid` 回收） | 子进程退出无僵尸 |
| D17 | `initialize` / `initialized` 握手 + 能力协商 | 日志能看到协商结果 |
| D18 | `tools/list` 转发与聚合 | 返回两个 server 的工具清单 |
| D19 | `tools/call` 转发 + 请求 ID 映射 | 调用结果正确回传 |
| D20 | 命名冲突策略（`server.tool`）+ 错误透传 | 同名工具不冲突 |
| D21 | 集成测试：Python mock MCP server | 端到端用例通过 |

**W3 验收**：真实 MCP 客户端能通过网关调用下游工具；协议消息不污染日志。

### W4：背压、指标、压测、投递

| 日 | 任务 | 验收 |
|---|---|---|
| D22 | SPSC 有界队列接入请求路径，满则 429，记录丢弃计数 | 压测时能看到背压生效 |
| D23 | `/metrics`（请求数、错误、延迟直方图、队列深度） | `curl /metrics` 有数据 |
| D24 | 压测基线：wrk 在本机打 VM，记录吞吐/P95/P99 | 原始数据落到 `benchmarks/results/` |
| D25 | ASan/TSan 全量跑 + 修问题 | 两种 sanitizer 干净 |
| D26 | README、架构图、演示脚本（一键起后端 + 网关 + curl 演示） | 别人照 README 能复现 |
| D27 | 简历更新 + 面试问答演练 | 每个模块能讲 5 分钟 |
| D28 | 投递 | 至少投 10 家 |

---

## 3. 验收命令

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/aegis --help

# sanitizer 构建
cmake -S . -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
cmake --build build-asan -j && ctest --test-dir build-asan

# 压测（macOS 侧打 VM）
wrk -t4 -c100 -d30s http://<vm-ip>:8080/api/echo
```

---

## 4. 技术决策（面试要能背）

| 决策 | 选择 | 理由 | 放弃了什么 |
|---|---|---|---|
| 网络库 | Boost.Asio | 异步取消、定时器、跨平台成熟 | 自己写 reactor 的教学价值 |
| HTTP | Boost.Beast | 协议正确性（chunked、keep-alive） | 自研解析器的“深度” |
| 并发模型 | 多线程 + strand | 每 session 串行化，避免全局锁 | 单线程极致简单 |
| 背压 | 有界 SPSC + 429 | 保护内存、延迟可控 | 无界队列的“不丢请求” |
| MCP 传输 | stdio（换行 JSON） | 官方默认、无需网络 | 远程 SSE/HTTP 传输 |
| 配置 | TOML | 可读、支持注释 | JSON（无注释） |

---

## 5. 风险与降级

| 风险 | 概率 | 降级方案 |
|---|---|---|
| Asio 生命周期 bug 拖垮 W1 | 中 | 严格照 Beast 官方示例结构；每个 session 一个类 |
| W2 代理跑不通 | 中 | 暂停 MCP，先保证 HTTP 完整；投递版本只写 HTTP |
| MCP 规范细节踩坑 | 高 | 只做 `initialize`/`tools/list`/`tools/call`，其余不做 |
| 子进程僵尸/管道死锁 | 中 | 独立线程收尸；写端关闭要显式 |
| 时间不足 | 高 | 砍掉限流、健康检查、热更新；保 HTTP + 测试 + 压测 |

---

## 6. 简历模板（只写已验证的）

> **Aegis Gateway**｜C++20 / Boost.Asio / Beast｜个人项目｜2026.11
> - 基于 Asio 实现异步 HTTP 反向代理，strand 串行化会话回调，支持多上游路由、超时与取消；
> - 实现 MCP 工具网关：子进程 stdio 会话、`tools/list` 聚合与 `tools/call` 路由，请求 ID 精确匹配；
> - 有界 SPSC 队列承载请求背压，队列满返回 429，指标暴露队列深度与丢弃数；
> - GoogleTest 单元/集成测试 + 故障注入，ASan/TSan 干净；wrk 压测 P99 [X] ms。

---

## 7. 面试题（每模块 5 问）

**Asio**
1. `io_context` 和线程是什么关系？多个线程 `run()` 安全吗？
2. `strand` 解决什么问题？不用会怎样？
3. 异步回调里为什么常用 `shared_from_this`？什么时候会循环引用？
4. `cancel()` 之后回调还会执行吗？错误码是什么？
5. 定时器怎么实现超时？为什么不用 `sleep`？

**HTTP 代理**
6. keep-alive 下连接怎么复用？什么情况要关连接？
7. `Content-Length` 和 chunked 怎么处理？
8. 上游返回 4xx/5xx，网关该原样透传还是改写？
9. 502 和 504 的区别？各自触发条件？
10. 大请求体怎么避免占满内存？

**MCP**
11. JSON-RPC 的 id 有什么用？通知和请求怎么区分？
12. `initialize` 里协商了什么？版本不匹配怎么办？
13. stdio 传输为什么用换行分隔而不是长度前缀？
14. 下游 server 崩溃，网关怎么感知并清理？
15. 两个 server 工具重名怎么处理？

**并发与性能**
16. 背压策略有哪些？为什么选 429？
17. SPSC 队列为什么比 mutex 队列快？伪共享怎么避免？
18. 怎么测 P99？为什么平均值会骗人？
19. 压测时客户端和服务端在同一台机器上有什么问题？
20. 你怎么证明优化有效？

---

## 8. 参考资料

- Boost.Asio 文档：异步 TCP、`strand`、定时器、cancellation
- Boost.Beast 官方示例：async server / client
- MCP 规范：Lifecycle、Tools、Transports、Cancellation
- GoogleTest 文档
- 《C++ Concurrency in Action》第 5、7 章（内存序与无锁队列）
