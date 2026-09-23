"""本机集成测试：动态端口、有限等待、失败时回收子进程。"""
import contextlib
from pathlib import Path
import selectors
import socket
import struct
import subprocess

ROOT = Path(__file__).resolve().parent

@contextlib.contextmanager
def server(program):
    args = [str(ROOT / program)]
    args += ["0"] if program == "tcp_c17" else ["server", "0"]
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        with selectors.DefaultSelector() as sel:
            sel.register(p.stdout, selectors.EVENT_READ)
            if not sel.select(5):
                raise AssertionError("server startup timeout")
            line = p.stdout.readline()
        if not line.startswith("PORT "):
            raise AssertionError(f"startup failed: {line} {p.stderr.read()}")
        yield p, int(line.split()[1])
    finally:
        if p.poll() is None:
            p.kill()
        p.communicate(timeout=5)

def run(*args):
    return subprocess.run([str(ROOT / args[0]), *args[1:]], capture_output=True,
                          text=True, timeout=6, check=True).stdout

def finish(p, expected=0):
    assert p.wait(timeout=5) == expected

def exact(s, count):
    out = b""
    while len(out) < count:
        chunk = s.recv(count - len(out))
        if not chunk:
            raise AssertionError("unexpected EOF")
        out += chunk
    return out

with server("tcp_c17") as (p, port):
    with socket.create_connection(("127.0.0.1", port), timeout=3) as s:
        payload = bytes(range(256)) * 32
        s.sendall(payload)
        s.shutdown(socket.SHUT_WR)
        assert exact(s, len(payload)) == payload
        assert s.recv(1) == b""
    finish(p)
print("PASS C17 binary echo and EOF")

with server("tcp_cpp20") as (p, port):
    assert run("tcp_cpp20", "client", str(port), "你好 C++20") == "你好 C++20\n"
    finish(p)
print("PASS C++20 client and half-close")

with server("tcp_cpp20") as (p, port):
    with socket.create_connection(("127.0.0.1", port), timeout=3) as s:
        for payload in (b"", b"a\x00b", bytes(range(256)) * 1024):
            header = struct.pack("!I", len(payload))
            # 单字节提交头部；TCP 可合并它们，测试不依赖内核如何分段。
            for byte in header:
                s.sendall(bytes([byte]))
            s.sendall(payload)
            length = struct.unpack("!I", exact(s, 4))[0]
            assert exact(s, length) == payload
        # 一次写入两个完整帧，检查连续解析。
        s.sendall(struct.pack("!I", 1) + b"x" + struct.pack("!I", 1) + b"y")
        assert exact(s, 10) == struct.pack("!I", 1) + b"x" + struct.pack("!I", 1) + b"y"
        s.shutdown(socket.SHUT_WR)
        assert s.recv(1) == b""
    finish(p)
print("PASS framing: empty, binary, large, fragmented input, multiple frames")

for bad in (b"\x00\x00", struct.pack("!I", 3) + b"a", struct.pack("!I", 1048577)):
    with server("tcp_cpp20") as (p, port):
        with socket.create_connection(("127.0.0.1", port), timeout=3) as s:
            s.sendall(bad)
            s.shutdown(socket.SHUT_WR)
            finish(p, 1)
print("PASS truncated header/body and oversized frame rejected")

for text in ("hello UDP", ""):
    with server("udp_cpp20") as (p, port):
        assert run("udp_cpp20", "client", str(port), text) == text + "\n"
        finish(p)
print("PASS UDP normal and zero-length datagrams")
assert "127.0.0.1:80" in run("api_lab", "resolve", "127.0.0.1", "80")
assert "linger enabled=" in run("api_lab", "options")
assert "copied=7 truncated=1 data=HEAD:bo" in run("api_lab", "msg")
assert "mark=1 byte=!" in run("api_lab", "oob")
print("PASS resolver, options, sendmsg/recvmsg truncation, OOB mark")
