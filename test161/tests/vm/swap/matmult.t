---
name: "Matrix Multiplication (Swap)"
description: >
  Test page faults by performing matrix multiplication on a large array.
tags: [swap-basic, swap]
depends: [/vm/matmult.t]
sys161:
  cpus: 2
  ram: 1M
  disk1:
    enabled: true
monitor:
  commandtimeout: 70.0
  window: 40
misc:
  prompttimeout: 3600.0
stat:
  resolution: 0.1
---
| p /testbin/matmult
