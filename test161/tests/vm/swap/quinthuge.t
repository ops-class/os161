---
name: "Quint Huge (Swap)"
description: >
  Run five concurent copies of huge.
tags: [swap]
depends: [swap-basic, /vm/quinthuge.t, shell]
sys161:
  cpus: 2
  ram: 2M
  disk1:
    enabled: true
monitor:
  progresstimeout: 20.0
  commandtimeout: 3530.0
  window: 20
misc:
  prompttimeout: 3600.0
stat:
  resolution: 0.2
---
khu
$ /testbin/quinthuge
khu
