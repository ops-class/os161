---
name: "sbrk Badcall"
description: >
  "Tests sbrk error conditions"
tags: [sbrk]
depends: [not-dumbvm-vm, shell]
---
$ /testbin/badcall h
