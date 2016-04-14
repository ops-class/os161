---
name: "Stack Test"
description: >
  Tests your VM stack handling by running the bigexec test from ASST2.
tags: [vm]
depends: [not-dumbvm-vm, shell]
sys161:
  ram: 2M
---
$ /testbin/bigexec
$ /testbin/stacktest
