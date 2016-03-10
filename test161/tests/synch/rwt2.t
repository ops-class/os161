---
name: "RW Lock Test 2"
description:
  Tests that reader-writer locks allow maximum read concurrency when no
  writers are waiting.
tags: [synch, rwlocks, kleaks]
depends: [boot, semaphores, cvs]
sys161:
  cpus: 32
---
khu
rwt2
khu
