#!/usr/bin/env python3
"""实验 1 用的最小 echo 服务器：配合 strace 观察 accept/read/write。"""

import socket
import sys

port = int(sys.argv[1]) if len(sys.argv) > 1 else 9001

server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", port))
server.listen()
print(f"listening on {port}", flush=True)

while True:
    client, addr = server.accept()
    print("accepted", addr, flush=True)
    while True:
        data = client.recv(1024)
        if not data:
            break
        client.sendall(data)
    client.close()
    print("closed", addr, flush=True)
