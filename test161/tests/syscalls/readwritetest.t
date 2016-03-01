---
name: "Read and Write Test"
description: >
  Tests sys_read and sys_write by reading and writing to a file.
tags: [sys_read,sys_write,filesyscalls,syscalls]
depends: [console,sys_open]
sys161:
  ram: 512K
---
p /testbin/readwritetest
