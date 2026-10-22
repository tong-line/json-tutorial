// Lambda 五讲：编译运行后逐段看输出
//   g++ -std=c++20 -Wall -Wextra labs/cpp_lambda.cpp -o /tmp/lambda && /tmp/lambda

#include <algorithm>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------
// 1. 基本形式：一个没有名字的函数
//    [捕获](参数) -> 返回类型 { 函数体 }
//    -> 返回类型 可以省略，编译器会推导
// ---------------------------------------------------------------
static void lab1_basic() {
    std::puts("\n== Lab 1: 基本形式 ==");

    // 定义：把 lambda 存进 auto 变量
    auto add = [](int a, int b) { return a + b; };
    std::printf("add(2, 3) = %d\n", add(2, 3));

    // 不需要捕获任何外部变量时，[] 里是空的
    auto square = [](int x) { return x * x; };
    std::printf("square(7) = %d\n", square(7));
}

// ---------------------------------------------------------------
// 2. 捕获：lambda 怎么"看到"外面的变量
//    [x]  按值：拷贝一份，之后外面怎么改都不影响
//    [&x] 按引用：直接操作外面的变量
//    [=]  所有用到的变量按值捕获
//    [&]  所有用到的变量按引用捕获
// ---------------------------------------------------------------
static void lab2_capture() {
    std::puts("\n== Lab 2: 捕获 ==");

    int x = 1;
    auto by_value = [x] { std::printf("by_value: %d\n", x); };
    auto by_ref = [&x] { std::printf("by_ref:   %d\n", x); };

    x = 2;  // 两个 lambda 定义完之后，再改 x

    by_value();  // 打印 1：捕获时就拷走了
    by_ref();    // 打印 2：看到的是最新的 x
}

// ---------------------------------------------------------------
// 3. 当回调传：C 的函数指针 vs C++ 的 std::function
//    函数指针只能接收"不捕获"的 lambda；
//    std::function 可以接收任何可调用对象（含捕获的 lambda）
// ---------------------------------------------------------------
static void call_twice_ptr(void (*f)(int)) {
    f(1);
    f(2);
}

static void call_twice_fn(const std::function<void(int)>& f) {
    f(1);
    f(2);
}

static void lab3_callback() {
    std::puts("\n== Lab 3: 当回调传 ==");

    // 不捕获的 lambda 可以转成函数指针
    call_twice_ptr([](int v) { std::printf("ptr  : %d\n", v); });

    int base = 100;
    // 捕获了变量的 lambda 不能转函数指针，但可以转 std::function
    call_twice_fn([base](int v) { std::printf("fn   : %d\n", v + base); });
}

// ---------------------------------------------------------------
// 4. 捕获 this 与 shared_ptr —— Asio 异步回调的标准姿势
//    [this] 只保存裸指针，对象销毁后就是悬空指针
//    [self] 保存 shared_ptr，回调未执行完对象就不会销毁
// ---------------------------------------------------------------
struct Session : std::enable_shared_from_this<Session> {
    void start() {
        auto self = shared_from_this();  // 拿到自己的 shared_ptr
        // 同时捕获 this（访问成员）和 self（延长生命周期）
        auto callback = [this, self] {
            std::printf("Session 还活着，use_count = %ld\n", self.use_count());
        };
        callback();
        std::printf("start() 结束，use_count = %ld\n", use_count());
    }
};

static void lab4_shared() {
    std::puts("\n== Lab 4: 捕获 this 与 shared_ptr ==");

    auto s = std::make_shared<Session>();
    std::printf("创建后 use_count = %ld\n", s.use_count());
    s->start();
    std::printf("start 返回后 use_count = %ld\n", s.use_count());
}

// ---------------------------------------------------------------
// 5. 危险写法：按引用捕获局部变量，然后把 lambda 存起来
//    局部变量销毁后，lambda 里的引用就悬空了。
//    （这里只演示"值捕获"的安全行为，悬空案例见注释）
// ---------------------------------------------------------------
static void lab5_lifetime() {
    std::puts("\n== Lab 5: 生命周期 ==");

    std::function<void()> safe;
    {
        int local = 42;
        // 按值捕获：local 的副本被存进 lambda，出了作用域仍然有效
        safe = [local] { std::printf("safe: %d\n", local); };
    }
    safe();  // 打印 42

    // 如果写成 [&local]，这里就是悬空引用（未定义行为），不要模仿。
}

int main() {
    lab1_basic();
    lab2_capture();
    lab3_callback();
    lab4_shared();
    lab5_lifetime();
}
