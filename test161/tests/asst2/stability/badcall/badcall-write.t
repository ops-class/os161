---
name: "Badcall-write Test"
description:
  Tests whether write syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
---
$ /testbin/badcall e
