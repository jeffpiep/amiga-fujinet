---
name: project-appkey-hold
description: AppKey contract on hold pending Mark Fisher's app-store implementation in fujinet-nio/nio-lib
metadata:
  type: project
---

AppKey contract (`contracts/fujinet-appkey-protocol.md`, on `worktree-track1b`) is on hold.

**Why:** Mark Fisher (fujinet-nio upstream maintainer) is implementing an "app-store" feature in both `fujinet-nio` and `fujinet-nio-lib` that covers the same capability. He's also adding a compatibility layer to map old appkey names to the new system. Revisit once Mark shares details.

**How to apply:** Don't implement appkey support in fujinet-nio or wire up the serial path in fujinet-compat-amiga until Mark's app-store design is understood. The existing local-file fallback in `fn_fuji.c` is sufficient for emulator testing in the meantime.
