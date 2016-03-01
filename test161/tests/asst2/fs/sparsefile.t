---
name: "Sparsefile Test"
description:
  Tests lseek to write one byte of data after skipping the first x bytes
  in an empty file. Then attempts to read this byte.
tags: [fs]
depends: [shell, /asst2/fs/fileonlytest.t]
sys161:
  ram: 2M
---
$ /testbin/sparsefile test 1048
