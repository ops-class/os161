---
name: "Badcall-waitpid Test"
description:
  Tests whether waitpid syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
sys161:
  ram: 4M
---
$ /testbin/badcall b
