// 非阻塞实验：c 语言 + 管道，不需要网络
//   gcc -O2 -Wall -Wextra labs/nonblocking_demo.c -o /tmp/nb
//   timeout 2 /tmp/nb      ← timeout 会在 2 秒后杀掉卡住的程序

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void show_flags(int fd, const char* label) {
    int flags = fcntl(fd, F_GETFL, 0);
    printf("%s: flags=0x%x, O_NONBLOCK %s\n", label, flags,
           (flags & O_NONBLOCK) ? "已设置" : "未设置");
}

int main(void) {
    int fds[2];
    pipe(fds);  /* fds[0] 读端，fds[1] 写端 */
    char buf[16];

    show_flags(fds[0], "初始");

    /* 第一步：读出当前 flags，再把 O_NONBLOCK 这一位加上 */
    int flags = fcntl(fds[0], F_GETFL, 0);
    fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);
    show_flags(fds[0], "设置后");

    /* 第二步：没有数据时读，看会发生什么 */
    errno = 0;
    ssize_t n = read(fds[0], buf, sizeof buf);
    printf("无数据时 read 返回 %zd, errno=%d (%s)\n", n, errno, strerror(errno));

    /* 第三步：写入 5 字节，再读 */
    write(fds[1], "hello", 5);
    n = read(fds[0], buf, sizeof buf);
    printf("有数据时 read 返回 %zd, 内容=%.*s\n", n, (int)n, buf);

    /* 第四步：读空之后再读一次 */
    errno = 0;
    n = read(fds[0], buf, sizeof buf);
    printf("再读一次返回 %zd, errno=%d (%s)\n", n, errno, strerror(errno));

    /* 第五步：把 O_NONBLOCK 去掉，恢复阻塞 */
    flags = fcntl(fds[0], F_GETFL, 0);
    fcntl(fds[0], F_SETFL, flags & ~O_NONBLOCK);
    show_flags(fds[0], "恢复阻塞");

    printf("下面这次 read 会卡住（管道里没数据），用 timeout 2 跑就能看到\n");
    fflush(stdout);

    n = read(fds[0], buf, sizeof buf);
    printf("居然读到了 %zd 字节\n", n);
    return 0;
}
