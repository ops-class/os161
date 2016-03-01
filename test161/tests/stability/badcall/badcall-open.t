---
name: "Badcall-open Test"
description:
  Tests whether open syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
---
$ /testbin/badcall c
