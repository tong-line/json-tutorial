#!/usr/bin/env python3
"""验证两个 echo 服务器：阻塞版单连接、epoll 版双连接并发。"""

import socket
import subprocess
import time


def start(path: str, port: int) -> subprocess.Popen:
    proc = subprocess.Popen(
        [path, str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    time.sleep(0.4)
    return proc


def main() -> None:
    blk = start("/tmp/blk", 9101)
    ep = start("/tmp/ep", 9102)

    try:
        s = socket.create_connection(("127.0.0.1", 9101), timeout=2)
        s.sendall(b"hello blocking\n")
        print("blocking:", s.recv(100))
        s.close()

        # 两个连接同时保持打开，验证 epoll 版确实是并发处理
        c1 = socket.create_connection(("127.0.0.1", 9102), timeout=2)
        c2 = socket.create_connection(("127.0.0.1", 9102), timeout=2)
        c1.sendall(b"one\n")
        c2.sendall(b"two\n")
        print("epoll:", c1.recv(100), c2.recv(100))
        c1.close()
        c2.close()
    finally:
        for proc in (blk, ep):
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()

        print("--- blocking server log ---")
        print(blk.stdout.read() if blk.stdout else "")
        print("--- epoll server log ---")
        print(ep.stdout.read() if ep.stdout else "")


if __name__ == "__main__":
    main()
