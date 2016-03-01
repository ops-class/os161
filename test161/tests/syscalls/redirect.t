---
name: "Redirect Test"
description: >
  Tests sys_dup2. Opens a file handle, then forks a new process which
  manipulates the file table.
tags: [syscalls,filesyscalls]
depends: [/asst2/process/forktest.t]
sys161:
  ram: 512K
---
p /testbin/redirect
