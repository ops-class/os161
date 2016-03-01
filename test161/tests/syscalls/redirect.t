---
name: "Redirect Test"
description: >
  Tests sys_dup2. Opens a file handle, then forks a new process which
  manipulates the file table.
tags: [sys_dup2,filesyscalls,syscalls]
depends: [console,sys_fork]
sys161:
  ram: 1M
---
p /testbin/redirect
