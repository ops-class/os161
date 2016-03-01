---
name: "Crash-all Shell Test"
description:
  Tests whether your system correctly handles a variety of
  illegal attempts from userspace processes to access resources that
  do not belong to it. Please see userland/testbin/crash for more info.
  Runs it from the shell.
tags: [stability, crash, crash-fork]
depends: [shell, /asst2/process/forktest.t]
sys161:
  ram: 4M
---
$ /testbin/crash *
