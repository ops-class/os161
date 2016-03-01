---
name: "Console Test"
description: >
  Make sure that the console works.
tags: [console]
depends: [boot]
sys161:
  ram: 512K
---
p /testbin/consoletest
