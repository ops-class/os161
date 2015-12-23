/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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

#define TESTFILE "badcallfile"
#define TESTDIR  "badcalldir"
#define TESTLINK "badcalllink"

#if defined(__clang__) || defined(__GNUC__)
#define PF(a, b) __attribute__((__format__(__printf__, a, b)))
#else
#define PF(a, b)
#endif

/* driver.c */
int open_testfile(const char *str);
int reopen_testfile(int openflags);
int create_testfile(void);
int create_testdir(void);
int create_testlink(void);

/* report.c */
PF(1, 2) void report_begin(const char *descfmt, ...);
void report_hassubs(void);
PF(1, 2) void report_beginsub(const char *descfmt, ...);
PF(1, 2) void report_warn(const char *fmt, ...);
PF(1, 2) void report_warnx(const char *fmt, ...);
void report_result(int rv, int error);
void report_saw_enosys(void);
void report_passed(void);
void report_failure(void);
void report_skipped(void);
void report_aborted(void);
void report_survival(int rv, int error);
void report_check(int rv, int error, int right_error);
void report_check2(int rv, int error, int okerr1, int okerr2);
void report_check3(int rv, int error, int okerr1, int okerr2, int okerr3);

/* common_buf.c */
void test_read_buf(void);
void test_write_buf(void);
void test_getdirentry_buf(void);
void test_getcwd_buf(void);
void test_readlink_buf(void);

/* common_fds.c */
void test_read_fd(void);
void test_write_fd(void);
void test_close_fd(void);
void test_ioctl_fd(void);
void test_lseek_fd(void);
void test_fsync_fd(void);
void test_ftruncate_fd(void);
void test_fstat_fd(void);
void test_getdirentry_fd(void);
void test_dup2_fd(void);

/* common_path.c */
void test_open_path(void);
void test_remove_path(void);
void test_rename_paths(void);
void test_link_paths(void);
void test_mkdir_path(void);
void test_rmdir_path(void);
void test_chdir_path(void);
void test_symlink_paths(void);
void test_readlink_path(void);
void test_stat_path(void);
void test_lstat_path(void);

/* bad_*.c */
void test_execv(void);
void test_waitpid(void);
void test_open(void);
void test_read(void);
void test_write(void);
void test_close(void);
void test_reboot(void);
void test_sbrk(void);
void test_ioctl(void);
void test_lseek(void);
void test_fsync(void);
void test_ftruncate(void);
void test_fstat(void);		/* in bad_stat.c */
void test_remove(void);
void test_rename(void);
void test_link(void);
void test_mkdir(void);
void test_rmdir(void);
void test_chdir(void);
void test_getdirentry(void);
void test_symlink(void);
void test_readlink(void);
void test_dup2(void);
void test_pipe(void);
void test_time(void);
void test_getcwd(void);
void test_stat(void);
void test_lstat(void);		/* in bad_stat.c */
