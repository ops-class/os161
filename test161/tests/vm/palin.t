---
name: "Taco cat"
description: >
  Test page faults by ensuring a known palindrone stored in static
  memory is actually a palindrone.
tags: [vm]
depends: [not-dumbvm-paging]
---
| p /testbin/palin
