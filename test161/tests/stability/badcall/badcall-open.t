---
name: "Bad Open"
description:
  Stability test for sys_open.
tags: [badcall,stability]
depends: [shell]
sys161:
  ram: 512K
---
$ /testbin/badcall c
