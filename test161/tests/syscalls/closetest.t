---
name: "Close Test"
description: >
  Tests sys_close by closing STDIN and a normal file.
tags: [syscalls,filesyscalls]
depends: [boot, /asst2/fs/opentest.t]
sys161:
  ram: 512K
---
p /testbin/closetest
