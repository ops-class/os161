---
name: "Bad Wait"
description:
  Stability test for sys_wait.
tags: [badcall,stability]
depends: [shell]
sys161:
  ram: 2M
---
$ /testbin/badcall b
