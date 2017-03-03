---
name: "Calm Like a Fork Bomb"
tags: [stability]
depends: [console,sys_fork]
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
---
p /testbin/forkbomb
