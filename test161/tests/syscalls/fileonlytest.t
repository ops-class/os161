---
name: "Fileonly Test"
description: >
  Tests sys_{open,close,read,write,lseek}. Opens a file, writes 512
  bytes, skips 512 bytes and writes another 512 bytes (odd steps). Seeks
  back to beginning and writes the even steps. Then seeks back to the
  beginning and verifies that the contents are correct.
tags: [filesyscalls,syscalls]
depends: [console]
sys161:
  ram: 512K
---
p /testbin/fileonlytest
