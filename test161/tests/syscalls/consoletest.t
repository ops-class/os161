---
name: "Console Test"
description: >
  Make sure that the console works and the kernel menu waits appropriately.
tags: [console]
depends: [boot]
sys161:
  ram: 512K
---
p /testbin/consoletest
