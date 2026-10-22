# Aegis Gateway

C++20 异步 HTTP 反向代理 + MCP 工具网关（开发中）。

## 状态

- [x] W1 D1：工程骨架（CMake + Boost.Asio + GoogleTest + CI）
- [ ] W1 D2：异步 echo server
- [ ] W2：HTTP 反向代理（硬门槛）
- [ ] W3：MCP 工具网关
- [ ] W4：背压、指标、压测、投递

详细计划见 [PLAN.md](PLAN.md)。

## 构建与测试

依赖：CMake ≥ 3.20、C++20 编译器、Boost ≥ 1.74（system）、GoogleTest。

```bash
sudo apt install cmake g++ libboost-system-dev libgtest-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/aegis
```

## 目录

```
include/aegis/   公共头文件（core / http / mcp / runtime）
src/             实现
tests/unit/      单元测试
config/          配置样例
```
