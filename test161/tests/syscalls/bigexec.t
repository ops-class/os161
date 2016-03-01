---
name: "Bigexec Test"
description:
  Tests to ensure that the argument passing logic is not hard-coded
  to small argument lengths.
tags: [sys_exec,procsyscalls,syscalls]
depends: [shell]
sys161:
  ram: 4M
---
$ /testbin/bigexec
