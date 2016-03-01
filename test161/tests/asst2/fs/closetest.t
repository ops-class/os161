---
name: "Close Test"
description:
  Tests close syscall. Attempts to close stdin and a normal file.
tags: [fs]
depends: [boot, /asst2/fs/opentest.t]
---
p /testbin/closetest
