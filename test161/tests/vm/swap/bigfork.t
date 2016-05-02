---
name: "Big Fork (Swap)"
description: >
  Stress tests your VM system by performing various matrix computations in
  multiple concurrent processes. This test is a cross between parallelvm
  and forktest.
tags: [swap]
depends: [swap-basic, /vm/bigfork.t, shell]
sys161:
  cpus: 8
  ram: 2M
  disk1:
    enabled: true
monitor:
  progresstimeout: 40.0
  commandtimeout: 1330.0
  window: 40
misc:
  prompttimeout: 3600.0
stat:
  resolution: 0.1
---
khu
$ /testbin/bigfork
khu
