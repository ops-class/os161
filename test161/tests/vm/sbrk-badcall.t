---
name: "sbrk Badcall"
description: >
  "Tests sbrk error conditions"
tags: [sbrk]
depends: [not-dumbvm-paging, shell]
---
$ /testbin/badcall h
