---
name: "Faulter Test"
description:
  Tests whether kill_curthread is implemented correctly.
  Attempts to access an invalid memory address expecting the kernel
  to gracefully kill the current process instead of panicking.
tags: [proc]
depends: [console]
---
p /testbin/faulter
