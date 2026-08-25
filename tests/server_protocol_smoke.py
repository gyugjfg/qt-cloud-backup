#!/usr/bin/env python3
"""Protocol-level smoke test for the standalone Linux backup server."""

from __future__ import annotations

import hashlib
import pathlib
import socket
import subprocess
import sys
import tempfile
import time


def recv_until_close(sock: socket.socket) -> bytes:
    chunks = []
    while True:
        chunk = sock.recv(1024 * 1024)
        if not chunk:
            return b"".join(chunks)
        chunks.append(chunk)


def recv_line(sock: socket.socket) -> bytes:
    data = bytearray()
    while not data.endswith(b"\n"):
        chunk = sock.recv(1)
        if not chunk:
            raise AssertionError("server closed before line response")
        data.extend(chunk)
    return bytes(data).rstrip(b"\r\n")


def connect_to_server(root: pathlib.Path) -> tuple[subprocess.Popen[bytes], int]:
    server = pathlib.Path(sys.argv[1]).resolve()
    process = subprocess.Popen(
        [str(server), str(root)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    for _ in range(50):
        for port in range(10000, 10010):
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=0.1) as probe:
                    probe.sendall(b"filelist|./\n")
                    probe.recv(4096)
                return process, port
            except OSError:
                continue
        time.sleep(0.1)
    output = process.stdout.read().decode(errors="replace") if process.stdout else ""
    process.kill()
    raise AssertionError(f"server did not listen: {output}")


def main() -> int:
    payload = (b"cloud-backup smoke payload\n" * 4096) + bytes(range(251))
    expected_hash = hashlib.sha256(payload).hexdigest()

    with tempfile.TemporaryDirectory(prefix="cloud-backup-server-") as temp_dir:
        root = pathlib.Path(temp_dir)
        process, port = connect_to_server(root)
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
                client.sendall(f"fileput|./|smoke.bin|{len(payload)}|0\n".encode())
                assert recv_line(client).startswith(b"READY:0")
                client.sendall(payload)
                assert recv_line(client) == f"OK:{len(payload)}".encode()

            with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
                client.sendall(b"filelist|./\n")
                assert b"smoke.bin" in recv_until_close(client)

            with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
                client.sendall(b"filesave|smoke.bin\n")
                assert recv_line(client) == str(len(payload)).encode()

            with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
                client.sendall(f"filesave|smoke.bin|0|{len(payload) - 1}\n".encode())
                assert recv_line(client) == str(len(payload)).encode()
                client.sendall(b"OK\n")
                downloaded = client.recv(len(payload))
                while len(downloaded) < len(payload):
                    chunk = client.recv(len(payload) - len(downloaded))
                    if not chunk:
                        break
                    downloaded += chunk
                assert hashlib.sha256(downloaded).hexdigest() == expected_hash

            with socket.create_connection(("127.0.0.1", port), timeout=2) as client:
                client.sendall(b"filelist|../\n")
                assert recv_until_close(client).startswith(b"ERROR:")
        finally:
            process.terminate()
            try:
                process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=2)

    print(f"server protocol smoke passed: bytes={len(payload)} sha256={expected_hash}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
