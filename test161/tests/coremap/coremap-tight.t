---
name: "Coremap Test (Tight Bounds)"
description: >
  Allocates and frees all physical memory multiple times checking
  that the amount allocated is within a reasonable bound.
tags: [coremap]
depends: [not-dumbvm.t]
sys161:
  ram: 4M
---
| km5 --avail 20 --kernel 105
