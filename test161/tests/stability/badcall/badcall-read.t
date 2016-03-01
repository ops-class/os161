---
name: "Bad Read"
description:
  Stability test for sys_read.
tags: [badcall,stability]
depends: [shell]
sys161:
  ram: 2M
---
$ /testbin/badcall d
