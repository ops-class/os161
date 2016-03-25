---
name: "Basic kmalloc Test"
description: >
  Tests the kernel subpage allocator by allocating a large number of objects
  and freeing them somewhat later.
tags: [coremap]
depends: [not-dumbvm.t]
---
| km1
