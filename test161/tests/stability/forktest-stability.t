---
name: "Fork Stability Test"
description:
  Runs forktest 5 times to check for synchronization issues.
tags: [stability]
depends: [shell]
sys161:
  ram: 16M
---
$ /testbin/forktest
$ /testbin/forktest
$ /testbin/forktest
$ /testbin/forktest
$ /testbin/forktest
