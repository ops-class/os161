---
name: "Read and Write Test"
description: >
  Tests sys_read and sys_write by reading and writing to a file.
tags: [syscalls,filesyscalls]
depends: [/asst2/fs/opentest.t]
sys161:
  ram: 512K
---
p /testbin/readwritetest
