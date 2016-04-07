---
name: "Sort Test"
description: >
  Test page fault handling by sorting a large array.
tags: [vm]
depends: [not-dumbvm-vm]
sys161:
  ram: 2M
---
| p /testbin/sort
