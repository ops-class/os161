---
name: "Large kmalloc Test"
description: >
  Stresses the subpage allocator by allocating and freeing a large number of
  objects of various sizes.
tags: [coremap]
depends: [not-dumbvm.t]
---
| km3 5000
