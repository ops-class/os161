tags:
  - name: badcall
    desc: "All badcall tests for the various system calls"
  - name: boot
    desc: "Tests that check if your kernel can boot"
  - name: console
    desc: "Tests that check if you have a working userspace console"
  - name: coremap
    desc: "All coremap-related tests"
  - name: crash
    desc: "Tests that attempt to crash your kernel"
  - name: cvs
    desc: "Condition variable tests"
  - name: filesyscalls
    desc: "Filesystem syscall tests, e.g. read, write, open, close, etc."
  - name: kleaks
    desc: "Synch tests that also check for memory leaks"
  - name: locks
    desc: "Kernel lock tests"
  - name: not-dumbvm
    desc: "Tests that verify your coremap is not using dumbvm"
  - name: not-dumbvm-vm
    desc:  "Tests that verify your VM system is not using dumbvm"
  - name: proc
    desc: "Misc. process system call tests"
  - name: procsyscalls
    desc: "Tests that test process system calls, e.g. fork, exec, waitpid"
  - name: rwlocks
    desc: "Reader/writer lock tests"
  - name: sbrk
    desc: "sbrk tests"
  - name: semaphores
    desc: "Kernel semaphore tests"
  - name: shell
    desc: "Tests that check for a working userspace shell"
  - name: stability
    desc: "System call stability tests that stress test and check various error conditions"
  - name: stability-vm
    desc: "VM stress testing"
  - name: swap
    desc: "Tests that stress test your VM swapping implementation"
  - name: swap-basic
    desc: "Less stressful swap tests to ensure a swapping implementation"
  - name: synch
    desc: "All sychronization primitive tests"
  - name: synchprobs
    desc: "Synchronization problems that test your ablility to use synch primitives"
  - name: sys_close
    desc: "close() syscall tests"
  - name: sys_dup2
    desc: "dup2() syscall tests"
  - name: sys_exec
    desc: "exec() syscall tests"
  - name: sys_fork
    desc: "fork() syscall tests"
  - name: sys_lseek
    desc: "lseek() syscall tests"
  - name: sys_open
    desc: "open() syscall tests"
  - name: sys_read
    desc: "read() syscall tests"
  - name: sys_write
    desc: "write() syscall tests"
  - name: syscalls
    desc: "Tests that check the basic functionality of your system call implementations"
  - name: threads
    desc: "Kernel thread tests"
  - name: vm
    desc: "All non-swapping VM tests"

