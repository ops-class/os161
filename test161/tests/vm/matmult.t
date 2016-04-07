---
name: "Matrix Multiplication"
description: >
  Test page faults by performing matrix multiplication on a large array.
tags: [vm]
depends: [not-dumbvm-vm]
sys161:
  ram: 2M
---
| p /testbin/matmult
