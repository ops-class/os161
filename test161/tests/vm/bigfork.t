---
name: "Big Fork"
description: >
  Stress tests your VM by performing various matrix computations in
  multiple concurrent processes. This test is a cross between parallelvm
  and forktest.
tags: [vm]
depends: [not-dumbvm-paging, /syscalls/forktest.t]
sys161:
  cpus: 4
  ram: 8M
---
khu
$ /testbin/bigfork
khu
