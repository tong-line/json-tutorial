#!/usr/bin/env python3
"""对比 ET 读一次 vs ET 读到 EAGAIN：发送 10KB，看能收回多少。"""

import socket
import sys
import time


def run(port: int, label: str) -> None:
    payload = b"x" * 10000
    s = socket.create_connection(("127.0.0.1", port), timeout=3)
    s.sendall(payload)
    received = b""
    s.settimeout(1.5)
    try:
        while len(received) < len(payload):
            chunk = s.recv(65536)
            if not chunk:
                break
            received += chunk
    except socket.timeout:
        pass
    s.close()
    print(f"{label}: sent {len(payload)}, received {len(received)}")


if __name__ == "__main__":
    run(int(sys.argv[1]), sys.argv[2])
