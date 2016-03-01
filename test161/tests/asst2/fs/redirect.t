---
name: "Redirect Test"
description:
  Tests dup2. Opens a file handle, forks a new process and manipulates the file
  table using dup2.
tags: [fs]
depends: [/asst2/process/forktest.t]
sys161:
  ram: 4M
---
p /testbin/redirect
