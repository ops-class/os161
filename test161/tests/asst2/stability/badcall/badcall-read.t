---
name: "Badcall-read Test"
description:
  Tests whether read syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
---
$ /testbin/badcall d
