---
name: "Bad Exec"
description:
  Stability test for sys_exec.
tags: [badcall,stability]
depends: [shell]
sys161:
  ram: 2M
---
$ /testbin/badcall a
