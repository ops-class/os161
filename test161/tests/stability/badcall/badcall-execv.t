---
name: "Badcall-execv Test"
description:
  Tests whether execv syscall is implemented correctly by invoking it in all
  possible wrong ways.
tags: [badcall, stability]
depends: [shell]
sys161:
  ram: 4M
---
$ /testbin/badcall a
