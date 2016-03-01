---
name: "Badcall-dup2 Test"
description:
  Tests whether dup2 syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
---
$ /testbin/badcall w
