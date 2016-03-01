---
name: "Crash"
description: >
  Tests whether your system correctly handles a variety of bad process
  behavior.
tags: [crash,stability]
depends: [shell,sys_fork]
sys161:
  ram: 4M
---
$ /testbin/crash *
