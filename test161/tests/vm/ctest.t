---
name: "Link Hopper"
description: >
  Turn a huge array into a linked list and hop along the list triggering
  an interesting page fault pattern.
tags: [vm]
depends: [not-dumbvm-vm]
sys161:
  ram: 4M
---
| p /testbin/ctest
