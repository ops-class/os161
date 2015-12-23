/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _KERN_SYSCALL_H_
#define _KERN_SYSCALL_H_

/*
 * System call numbers.
 *
 * To foster compatibility, this file contains a number for every
 * more-or-less standard Unix system call that someone might
 * conceivably implement on OS/161. The commented-out ones are ones
 * we're pretty sure you won't be implementing. The others, you might
 * or might not. Check your own course materials to find out what's
 * specifically required of you.
 *
 * Caution: this file is parsed by a shell script to generate the assembly
 * language system call stubs. Don't add weird stuff between the markers.
 */

/*CALLBEGIN*/

//                              -- Process-related --
#define SYS_fork         0
#define SYS_vfork        1
#define SYS_execv        2
#define SYS__exit        3
#define SYS_waitpid      4
#define SYS_getpid       5
#define SYS_getppid      6
//                              (virtual memory)
#define SYS_sbrk         7
#define SYS_mmap         8
#define SYS_munmap       9
#define SYS_mprotect     10
//#define SYS_madvise    11
//#define SYS_mincore    12
//#define SYS_mlock      13
//#define SYS_munlock    14
//#define SYS_munlockall 15
//#define SYS_minherit   16
//                              (security/credentials)
#define SYS_umask        17
#define SYS_issetugid    18
#define SYS_getresuid    19
#define SYS_setresuid    20
#define SYS_getresgid    21
#define SYS_setresgid    22
#define SYS_getgroups    23
#define SYS_setgroups    24
#define SYS___getlogin   25
#define SYS___setlogin   26
//                              (signals)
#define SYS_kill         27
#define SYS_sigaction    28
#define SYS_sigpending   29
#define SYS_sigprocmask  30
#define SYS_sigsuspend   31
#define SYS_sigreturn    32
//#define SYS_sigaltstack 33
//                              (resource tracking and usage)
//#define SYS_wait4      34
//#define SYS_getrusage  35
//                              (resource limits)
//#define SYS_getrlimit  36
//#define SYS_setrlimit  37
//                              (process priority control)
//#define SYS_getpriority 38
//#define SYS_setpriority 39
//                              (process groups, sessions, and job control)
//#define SYS_getpgid    40
//#define SYS_setpgid    41
//#define SYS_getsid     42
//#define SYS_setsid     43
//                              (userlevel debugging)
//#define SYS_ptrace     44

//                              -- File-handle-related --
#define SYS_open         45
#define SYS_pipe         46
#define SYS_dup          47
#define SYS_dup2         48
#define SYS_close        49
#define SYS_read         50
#define SYS_pread        51
//#define SYS_readv      52
//#define SYS_preadv     53
#define SYS_getdirentry  54
#define SYS_write        55
#define SYS_pwrite       56
//#define SYS_writev     57
//#define SYS_pwritev    58
#define SYS_lseek        59
#define SYS_flock        60
#define SYS_ftruncate    61
#define SYS_fsync        62
#define SYS_fcntl        63
#define SYS_ioctl        64
#define SYS_select       65
#define SYS_poll         66

//                              -- Pathname-related --
#define SYS_link         67
#define SYS_remove       68
#define SYS_mkdir        69
#define SYS_rmdir        70
#define SYS_mkfifo       71
#define SYS_rename       72
#define SYS_access       73
//                              (current directory)
#define SYS_chdir        74
#define SYS_fchdir       75
#define SYS___getcwd     76
//                              (symbolic links)
#define SYS_symlink      77
#define SYS_readlink     78
//                              (mount)
#define SYS_mount        79
#define SYS_unmount      80


//                              -- Any-file-related --
#define SYS_stat         81
#define SYS_fstat        82
#define SYS_lstat        83
//                              (timestamps)
#define SYS_utimes       84
#define SYS_futimes      85
#define SYS_lutimes      86
//                              (security/permissions)
#define SYS_chmod        87
#define SYS_chown        88
#define SYS_fchmod       89
#define SYS_fchown       90
#define SYS_lchmod       91
#define SYS_lchown       92
//                              (file system info)
//#define SYS_statfs     93
//#define SYS_fstatfs    94
//#define SYS_getfsstat  95
//                              (POSIX dynamic system limits stuff)
//#define SYS_pathconf   96
//#define SYS_fpathconf  97

//                              -- Sockets and networking --
#define SYS_socket       98
#define SYS_bind         99
#define SYS_connect      100
#define SYS_listen       101
#define SYS_accept       102
//#define SYS_socketpair 103
#define SYS_shutdown     104
#define SYS_getsockname  105
#define SYS_getpeername  106
#define SYS_getsockopt   107
#define SYS_setsockopt   108
//#define SYS_recvfrom   109
//#define SYS_sendto     110
//#define SYS_recvmsg    111
//#define SYS_sendmsg    112

//                              -- Time-related --
#define SYS___time       113
#define SYS___settime    114
#define SYS_nanosleep    115
//#define SYS_getitimer  116
//#define SYS_setitimer  117

//                              -- Other --
#define SYS_sync         118
#define SYS_reboot       119
//#define SYS___sysctl   120

/*CALLEND*/


#endif /* _KERN_SYSCALL_H_ */
