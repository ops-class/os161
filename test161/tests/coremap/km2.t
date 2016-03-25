---
name: "kmalloc Stress Test"
description:
  "km2 stress tests the kernel subpage using the same approach as km1, but with multiple threads running concurrently."
tags: [coremap]
depends: [not-dumbvm.t]
---
|km2
