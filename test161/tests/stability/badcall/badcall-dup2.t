---
name: "Bad Dup"
description:
  Stability test for sys_dup2.
tags: [badcall,stability]
depends: [shell]
---
$ /testbin/badcall w
