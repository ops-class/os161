---
name: "Read-Write Test"
description:
  Tests read and write syscalls. Attempts to write a file and then
  read it back.
tags: [fs]
depends: [/asst2/fs/opentest.t]
---
p /testbin/readwritetest
