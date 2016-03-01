---
name: "Badcall-lseek Test"
description:
  Tests whether lseek syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
---
$ /testbin/badcall j
