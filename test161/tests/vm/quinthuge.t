---
name: "Quint Huge"
description: >
  Run five concurent copies of huge.
tags: [vm]
depends: [not-dumbvm-vm]
sys161:
  cpus: 2
  ram: 16M
monitor:
  progresstimeout: 15.0
---
khu
$ /testbin/quinthuge
khu
