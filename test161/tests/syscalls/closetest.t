---
name: "Close Test"
description: >
  Tests sys_close by closing STDIN and a normal file.
tags: [sys_close,filesyscalls,syscalls]
depends: [console,sys_open]
sys161:
  ram: 512K
---
p /testbin/closetest
