---
name: "sbrk Test"
description: >
  "Test various properites of your vm's heap management using
  the sbrk() system call."
tags: [sbrk]
depends: [not-dumbvm-vm, shell]
sys161:
  ram: 2M
monitor:
  progresstimeout: 15.0
---
khu
$ /testbin/sbrktest 1
$ /testbin/sbrktest 2
$ /testbin/sbrktest 3
$ /testbin/sbrktest 4
$ /testbin/sbrktest 5
$ /testbin/sbrktest 6
$ /testbin/sbrktest 7
$ /testbin/sbrktest 8
$ /testbin/sbrktest 11
$ /testbin/sbrktest 12
$ /testbin/sbrktest 13
$ /testbin/sbrktest 14
$ /testbin/sbrktest 15
$ /testbin/sbrktest 17
$ /testbin/sbrktest 22
$ /testbin/sbrktest 23
khu
