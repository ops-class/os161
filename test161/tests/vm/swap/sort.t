---
name: "Sort Test (Swap)"
description: >
  Test page fault handling by sorting a large array.
tags: [swap-basic, swap]
depends: [/vm/sort.t]
sys161:
  cpus: 2
  ram: 1M
  disk1:
    enabled: true
monitor:
  commandtimeout: 130.0
  window: 40
misc:
  prompttimeout: 3600.0
stat:
  resolution: 0.1
---
| p /testbin/sort
