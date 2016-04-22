---
name: "Calm Like a Fork Bomb (VM)"
tags: [stability-vm]
depends: [console, not-dumbvm-vm]
sys161:
  ram: 2M
monitor:
  progresstimeout: 40.0
  user:
    enablemin: true
    min: 0.001
    max: 1.0
  kernel:
    enablemin: true
    min: 0.01
commandoverrides:
  - name: /testbin/forkbomb
    timeout: 20.0
---
p /testbin/forkbomb
