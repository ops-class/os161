---
name: "Bad Dup"
description:
  Stability test for sys_dup2.
tags: [badcall,stability]
depends: [shell]
sys161:
  ram: 2M
---
$ /testbin/badcall w
