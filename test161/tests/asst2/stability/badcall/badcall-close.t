---
name: "Badcall-close Test"
description:
  Tests whether close syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
---
$ /testbin/badcall f
