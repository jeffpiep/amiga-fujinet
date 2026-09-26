#!/usr/bin/env python3
"""Guest-memory helpers over Amiberry IPC READ_MEM (one value per call)."""
import glob, os, socket, sys

def _sock():
    rt = os.environ.get("XDG_RUNTIME_DIR", f"/run/user/{os.getuid()}")
    return sorted(glob.glob(os.path.join(rt, "amiberry*.sock")))[0]

def cmd(*args):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(10)
    s.connect(_sock())
    s.sendall(("\t".join(str(a) for a in args) + "\n").encode())
    out = b""
    while not out.endswith(b"\n"):
        c = s.recv(65536)
        if not c:
            break
        out += c
    return out.decode(errors="replace").strip()

def rd(addr, width=4):
    r = cmd("READ_MEM", hex(addr), width).split("\t")
    if r[0] != "OK":
        raise RuntimeError(" ".join(r))
    return int(r[-1], 0)

def cstr(addr, maxlen=64):
    b = bytearray()
    for i in range(maxlen):
        c = rd(addr + i, 1)
        if c == 0:
            break
        b.append(c)
    return b.decode("latin-1")

def task(addr):
    """Print the interesting struct Task fields (NDK offsets)."""
    name = rd(addr + 10)
    print(f"task {addr:#x}: type={rd(addr + 8, 1)} pri={rd(addr + 9, 1)} "
          f"name={name:#x} '{cstr(name) if name else ''}'")
    print(f"  sigalloc={rd(addr + 18):#010x} sigwait={rd(addr + 22):#010x} "
          f"sigrecvd={rd(addr + 26):#010x}")
    print(f"  spreg={rd(addr + 54):#x} splower={rd(addr + 58):#x} "
          f"spupper={rd(addr + 62):#x} userdata={rd(addr + 88):#x}")

if __name__ == "__main__":
    what, arg = sys.argv[1], int(sys.argv[2], 0)
    {"task": task,
     "str": lambda a: print(cstr(a)),
     "long": lambda a: print(hex(rd(a))),
     "dump": lambda a: print(" ".join(f"{rd(a + 4 * i):08x}" for i in range(8)))}[what](arg)
