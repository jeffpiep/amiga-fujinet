#!/usr/bin/env python3
"""Send one tab-separated command to Amiberry's IPC socket and print the reply.

usage: ipc.py CMD [ARG ...]    e.g.  ipc.py PING   ipc.py SCREENSHOT /tmp/x.png
"""
import glob, os, socket, sys

rt = os.environ.get("XDG_RUNTIME_DIR", f"/run/user/{os.getuid()}")
socks = sorted(glob.glob(os.path.join(rt, "amiberry*.sock")))
if not socks:
    sys.exit(f"no amiberry socket in {rt}")
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.settimeout(10)
s.connect(socks[0])
s.sendall(("\t".join(sys.argv[1:]) + "\n").encode())
out = b""
try:
    while not out.endswith(b"\n"):
        chunk = s.recv(65536)
        if not chunk:
            break
        out += chunk
except socket.timeout:
    pass
print(out.decode(errors="replace").rstrip("\n"))
