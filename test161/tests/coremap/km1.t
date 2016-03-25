---
name: "Basic kmalloc Test"
description:
  "km1 tests the kernel's subpage allocator, allocating a large number of objects and freeing them somewhat later"
tags: [coremap]
depends: [not-dumbvm.t]
---
|km1
