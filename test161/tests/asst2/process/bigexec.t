---
name: "Bigexec Test"
description:
  Tests to ensure that the argument passing logic is not hard-coded
  to small argument lengths.
tags: [proc]
depends: [shell, /asst2/process/argtest.t]
sys161:
  ram: 4M
---
$ /testbin/bigexec
