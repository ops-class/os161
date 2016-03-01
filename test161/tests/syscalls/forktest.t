---
name: "Fork Test"
description: >
  Test that fork works.
tags: [syscalls,procsyscalls]
depends: [console, /asst2/fs/readwritetest.t]
sys161:
  ram: 4M
---
p /testbin/forktest
