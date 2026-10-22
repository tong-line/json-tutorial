// std::function 三讲：为什么需要它、怎么用、装进容器
//   g++ -std=c++20 -Wall -Wextra labs/cpp_function.cpp -o /tmp/fn && /tmp/fn

#include <cstdio>
#include <functional>
#include <vector>

// ---------------------------------------------------------------
// 0. 先看"C 风格"：函数指针只能装没有状态的函数
// ---------------------------------------------------------------
static void print_plain(int x) { std::printf("plain: %d\n", x); }

static void for_each_ptr(const std::vector<int>& v, void (*visit)(int)) {
    for (int x : v) visit(x);
}

// ---------------------------------------------------------------
// 1. std::function 版本：能装"带状态"的回调
//    把参数类型从 void(*)(int) 换成 const std::function<void(int)>&
// ---------------------------------------------------------------
static void for_each_fn(const std::vector<int>& v,
                        const std::function<void(int)>& visit) {
    for (int x : v) visit(x);
}

static void demo_basic() {
    std::puts("== 1. 函数指针 vs std::function ==");

    std::vector<int> v{1, 2, 3};

    // 函数指针：只能装不捕获的函数
    for_each_ptr(v, print_plain);

    // std::function：装一个捕获了 base 的 lambda
    int base = 100;
    std::function<void(int)> f = [base](int x) {
        std::printf("with base: %d\n", x + base);
    };
    for_each_fn(v, f);

    // 也可以直接把 lambda 传进去（自动转成 std::function）
    for_each_fn(v, [](int x) { std::printf("inline: %d\n", x * 2); });
}

// ---------------------------------------------------------------
// 2. 为什么函数指针装不下捕获的 lambda
//    （下面这行如果取消注释，编译会报错）
// ---------------------------------------------------------------
static void demo_why() {
    std::puts("\n== 2. 类型对比 ==");

    int base = 10;

    // void (*p)(int) = [base](int x) { std::printf("%d\n", x + base); };
    // 错误：捕获了 base 的 lambda 不能转成函数指针，
    //      因为函数指针没有地方存 base。

    std::function<void(int)> f = [base](int x) { std::printf("%d\n", x + base); };
    std::puts("std::function 可以，因为它内部会保存 base 的副本");
    f(5);  // 15
}

// ---------------------------------------------------------------
// 3. 真正好用的地方：把不同的回调装进同一个容器
// ---------------------------------------------------------------
static void demo_container() {
    std::puts("\n== 3. 装进容器 ==");

    int counter = 0;

    std::vector<std::function<void()>> handlers;
    handlers.push_back([&counter] { counter += 1; });
    handlers.push_back([&counter] { counter += 10; });
    handlers.push_back([&counter] { counter *= 2; });

    for (auto& h : handlers) h();

    std::printf("counter = %d\n", counter);  // (0+1+10)*2 = 22

    std::puts("三个 lambda 的类型各不相同，但都能装进 vector<std::function>");
}

int main() {
    demo_basic();
    demo_why();
    demo_container();
}
