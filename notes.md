# 学习笔记：从 C 到 C++ 重构 JSON 库

> 每章完成后的固定动作（3 分钟）：先自己做练习 → 对照 `*_answer` → 记录下面三条。
> 到第 8 章结束时，这份笔记就是 C++ 重构的 backlog。

## tutorial02 解析数字（D1）

- C 版别扭处：
- C++ 重构对应（提示：`from_chars`、locale 问题）：
- 踩到的边界条件（提示：`1e999`/ERANGE、`-0`、前导零、`1.`、`.1`、`1E012` 合法吗）：

## tutorial03 解析字符串

- C 版别扭处（提示：`realloc` 后指针失效、手动释放路径）：
- C++ 重构对应（提示：`std::string`、`variant`、Rule of 5）：
- 边界条件（提示：转义、`\u` 校验、嵌入 NUL）：

## tutorial04 Unicode

- C 版别扭处（提示：UTF-8 编码、surrogate pair、缓冲区）：
- C++ 重构对应（提示：`string_view`、`char8_t`/`u32string` 取舍）：
- 边界条件（提示：无效码点、高低代理对顺序）：

## tutorial05 解析数组

- C 版别扭处（提示：递归分配、泄漏路径、深度限制）：
- C++ 重构对应（提示：`vector<Value>`、异常安全、栈溢出防护）：
- 边界条件（提示：空数组、尾逗号、嵌套深度）：

## tutorial06 解析对象

- C 版别扭处（提示：`realloc` 后指针失效、重复 key、字符串哈希）：
- C++ 重构对应（提示：`map` vs `vector<pair>`、深拷贝语义）：
- 边界条件（提示：重复 key 处理、空对象、内存对齐）：

## tutorial07 生成器

- C 版别扭处（提示：多次拼接的 O(n²)、缓冲区管理）：
- C++ 重构对应（提示：`reserve`、`to_chars`、`std::string` 拼接）：
- 边界条件（提示：数字往返精度 `1e308`、转义输出、`-0`）：

## tutorial08 访问与其他功能

- C 版别扭处（提示：深拷贝手写递归、API 一致性）：
- C++ 重构对应（提示：拷贝/移动构造、`const` 重载、`operator[]`）：
- 边界条件（提示：自赋值、移动后状态、循环引用）：

---

## 服务器项目（W2 起）

面试问题清单见 `server-plan.md` 第 3 节，每完成一个里程碑勾掉对应问题。

## C++ 重构总 backlog（第 8 章后汇总）

- union + type tag → `std::variant`
- `malloc/realloc/free` → `string`/`vector`/`unique_ptr`
- 返回码 + 输出参数 → `Result<T, E>` / `std::expected`
- `const char*` + `strlen` → `string_view`
- 宏 → 内联函数/模板
- 递归深度限制（安全）
- 数字转换 locale 无关化
