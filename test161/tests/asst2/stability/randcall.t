---
name: "Randcall Test"
description:
  Invokes system calls on random with random arguments.
  Tests whether your system can gracefully recover from
  bad system calls.
tags: [randcall-2, stability]
depends: [shell]
sys161:
  ram: 16M
---
$ /testbin/randcall -f -c 1000 -r 30 2
