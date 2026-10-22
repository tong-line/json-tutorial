// add_fd 实验：注册 fd、看 epoll_event 的两个字段
//   gcc -O2 -Wall -Wextra labs/epoll_addfd_demo.c -o /tmp/addfd
//   /tmp/addfd

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

int main(void) {
    int ep = epoll_create1(0);
    int p[2];
    if (pipe(p) != 0) {
        perror("pipe");
        return 1;
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);       /* 清零：避免 padding 带垃圾 */
    ev.events = EPOLLIN;             /* 关心“可读” */
    ev.data.fd = p[0];               /* 用户数据：事件回来时原样带回 */

    int rc = epoll_ctl(ep, EPOLL_CTL_ADD, p[0], &ev);
    printf("sizeof(struct epoll_event) = %zu\n", sizeof ev);
    printf("第一次 ADD 返回 %d（0 = 成功）\n", rc);

    /* 重复 ADD 同一个 fd 应该失败 */
    rc = epoll_ctl(ep, EPOLL_CTL_ADD, p[0], &ev);
    printf("重复 ADD 返回 %d, errno=%d (%s)\n", rc, errno, strerror(errno));

    /* 写一个字节，触发 EPOLLIN */
    if (write(p[1], "x", 1) != 1) {
        return 1;
    }

    struct epoll_event out[4];
    int n = epoll_wait(ep, out, 4, 1000);
    printf("epoll_wait 返回 %d, events=0x%x, data.fd=%d\n", n, out[0].events,
           out[0].data.fd);

    /* 把 data 换成一个任意数字，看内核会不会原样带回 */
    ev.data.u64 = 0xdeadbeefULL;
    epoll_ctl(ep, EPOLL_CTL_MOD, p[0], &ev);

    if (write(p[1], "y", 1) != 1) {
        return 1;
    }
    n = epoll_wait(ep, out, 4, 1000);
    printf("MOD 后 epoll_wait 返回 %d, data.u64=0x%llx\n", n,
           (unsigned long long)out[0].data.u64);

    close(p[0]);
    close(p[1]);
    close(ep);
    return 0;
}
