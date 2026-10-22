// ET 实验：边缘触发 + 每次只读一次，会丢数据
//   gcc -O2 -Wall -Wextra -DREAD_ONCE labs/epoll_echo_et.c -o /tmp/et_once
//   gcc -O2 -Wall -Wextra               labs/epoll_echo_et.c -o /tmp/et_drain
//   /tmp/et_once 9201
//   /tmp/et_drain 9202

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int add_fd(int ep, int fd) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof ev);
    ev.events = EPOLLIN | EPOLLET;  /* ← 边缘触发 */
    ev.data.fd = fd;
    return epoll_ctl(ep, EPOLL_CTL_ADD, fd, &ev);
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int port = argc > 1 ? atoi(argv[1]) : 9200;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof addr) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(listen_fd, 16) < 0) {
        perror("listen");
        return 1;
    }
    set_nonblocking(listen_fd);

    int ep = epoll_create1(0);
    if (ep < 0) {
        perror("epoll_create1");
        return 1;
    }
    if (add_fd(ep, listen_fd) < 0) {
        perror("epoll_ctl");
        return 1;
    }

    printf("ET server listening on %d\n", port);

    struct epoll_event events[64];
    for (;;) {
        int n = epoll_wait(ep, events, 64, -1);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                for (;;) {
                    int client_fd = accept(listen_fd, NULL, NULL);
                    if (client_fd < 0) {
                        break;
                    }
                    set_nonblocking(client_fd);
                    if (add_fd(ep, client_fd) < 0) {
                        close(client_fd);
                        continue;
                    }
                    printf("accept fd=%d\n", client_fd);
                }
                continue;
            }

            int closed = 0;
#ifdef READ_ONCE
            /* 故意只读一次：ET 下剩余数据不会再触发事件 */
            char buf[1024];
            ssize_t k = read(fd, buf, sizeof buf);
            if (k > 0) {
                ssize_t written = 0;
                while (written < k) {
                    ssize_t w = write(fd, buf + written, (size_t)(k - written));
                    if (w <= 0) {
                        closed = 1;
                        break;
                    }
                    written += w;
                }
                printf("read once: %zd bytes\n", k);
            } else if (k == 0) {
                closed = 1;
            }
#else
            /* 正确写法：读到 EAGAIN */
            for (;;) {
                char buf[1024];
                ssize_t k = read(fd, buf, sizeof buf);
                if (k > 0) {
                    ssize_t written = 0;
                    while (written < k) {
                        ssize_t w = write(fd, buf + written, (size_t)(k - written));
                        if (w <= 0) {
                            closed = 1;
                            break;
                        }
                        written += w;
                    }
                    if (closed) {
                        break;
                    }
                } else if (k == 0) {
                    closed = 1;
                    break;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    closed = 1;
                    break;
                }
            }
#endif

            if (closed) {
                epoll_ctl(ep, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                printf("close fd=%d\n", fd);
            }
        }
    }

    close(ep);
    close(listen_fd);
    return 0;
}
