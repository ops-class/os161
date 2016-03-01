---
name: "Fileonly Test"
description:
  Tests open/close/read/write/lseek system calls.  Opens file, writes 512
  bytes, skips 512 bytes and writes another 512 bytes(odd steps). Then seeks
  back to beginning, and writes the even steps. Then seeks back to the
  beginning and verifies that the contents are correct.
tags: [fs]
depends: [console, /asst2/fs/readwritetest.t,]
sys161:
  ram: 4M
---
p /testbin/fileonlytest
