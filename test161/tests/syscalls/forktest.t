---
name: "Fork Test"
description: >
  Test that fork works.
tags: [sys_fork,procsyscalls,syscalls]
depends: [console]
sys161:
  ram: 2M
---
p /testbin/forktest
