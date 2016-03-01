---
name: "Sparsefile Test"
description: >
  Tests sys_lseek by writing one byte of data after skipping the beginning of
  an empty file and then attempting to read this byte.
tags: [syscalls,filesyscalls]
depends: [shell, /asst2/fs/fileonlytest.t]
sys161:
  ram: 512K
---
$ /testbin/sparsefile test 1048
