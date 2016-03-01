---
name: "Shell Test"
description:
  Tests whether the shell works by running /testbin/consoletest in the shell.
  The shell test tries to identify a race condition between the shell and the
  program that the shell is trying to run.
tags: [shell,procsyscalls,syscalls]
depends: [console]
sys161:
  ram: 1M
---
$ /testbin/shelltest
$ /testbin/shelltest
