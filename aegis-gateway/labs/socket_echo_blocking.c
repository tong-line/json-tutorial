// 实验 2：阻塞式 echo 服务器（一次只能服务一个连接）
//   gcc -O2 -Wall -Wextra labs/socket_echo_blocking.c -o /tmp/blk
//   /tmp/blk 9101

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);  /* 让日志立刻输出，方便观察 */
    int port = argc > 1 ? atoi(argv[1]) : 9101;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int yes = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    struct sockaddr_in addr = {0};
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

    printf("blocking server listening on %d\n", port);

    for (;;) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof peer;

        /* 注意：只有前一个连接彻底结束后，才会回到这里 accept 下一个 */
        int client_fd = accept(listen_fd, (struct sockaddr*)&peer, &peer_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        printf("client connected: fd=%d\n", client_fd);

        char buf[1024];
        ssize_t n;
        while ((n = read(client_fd, buf, sizeof buf)) > 0) {
            ssize_t written = 0;
            while (written < n) {
                ssize_t k = write(client_fd, buf + written, (size_t)(n - written));
                if (k <= 0) {
                    break;
                }
                written += k;
            }
        }

        printf("client disconnected: fd=%d\n", client_fd);
        close(client_fd);
    }
}
