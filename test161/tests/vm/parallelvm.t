---
name: "Parallel VM"
description: >
  Stress tests your VM by performing various matrix computations in
  multiple concurrent processes.
tags: [vm]
depends: [not-dumbvm-vm]
sys161:
  cpus: 2
  ram: 4M
---
| p /testbin/parallelvm
