---
name: "Smarter VM 2"
description: >
  Test whether you are using dumbvm by running the huge program,
  which dumbvm cannot run.
tags: [vm, not-dumbvm-vm]
depends: [console, not-dumbvm]
sys161:
  ram: 4M
---
| p /testbin/huge
