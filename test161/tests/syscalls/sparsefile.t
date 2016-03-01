---
name: "Sparsefile Test"
description: >
  Tests sys_lseek by writing one byte of data after skipping the beginning of
  an empty file and then attempting to read this byte.
tags: [sys_lseek,filesyscalls,syscalls]
depends: [shell]
sys161:
  ram: 1M
---
$ /testbin/sparsefile test 1048
