---
name: "VM Stability Test"
description:
  Runs various VM tests to test for synchronization issues.
tags: [stability-vm]
depends: [/vm/bigfork.t, /vm/parallelvm.t, /vm/quintmat.t, /vm/zero.t, /vm/sort.t]
sys161:
  ram: 10M
  cpus: 4
---
khu
$ /testbin/forktest
$ /testbin/quintmat
$ /testbin/forktest
$ /testbin/sort
$ /testbin/bigfork
$ /testbin/parallelvm
$ /testbin/quintmat
$ /testbin/forktest
$ /testbin/zero
khu
