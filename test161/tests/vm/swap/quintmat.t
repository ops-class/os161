---
name: "Quint Matrix Mult (Swap)"
description: >
  Run five concurent copies of matmult.
tags: [swap]
depends: [swap-basic, /vm/quintmat.t, shell]
sys161:
  cpus: 4
  ram: 2M
  disk1:
    enabled: true
monitor:
  commandtimeout: 400.0
  window: 40
misc:
  prompttimeout: 3600.0
stat:
  resolution: 0.1
---
khu
$ /testbin/quintmat
khu
