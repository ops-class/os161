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

#ifndef _LIB_H_
#define _LIB_H_

/*
 * Miscellaneous standard C functions for the kernel, and other widely used
 * kernel functions.
 *
 * Note: setjmp and longjmp are in <setjmp.h>.
 */


#include <cdefs.h>

/*
 * Assert macros.
 *
 * KASSERT and DEBUGASSERT are the same, except that they can be
 * toggled independently. DEBUGASSERT is used in places where making
 * checks is likely to be expensive and relatively unlikely to be
 * helpful.
 *
 * Note that there's also a COMPILE_ASSERT for compile-time checks;
 * it's in <cdefs.h>.
 *
 * Regular assertions (KASSERT) are disabled by the kernel config
 * option "noasserts". DEBUGASSERT could be controlled by kernel
 * config also, but since it's likely to be wanted only rarely during
 * testing there doesn't seem much point; one can just edit this file
 * temporarily instead.
 */
#include "opt-noasserts.h"

#if OPT_NOASSERTS
#define KASSERT(expr) ((void)(expr))
#else
#define KASSERT(expr) \
	((expr) ? (void)0 : badassert(#expr, __FILE__, __LINE__, __func__))
#endif

#if 1 /* no debug asserts */
#define DEBUGASSERT(expr) ((void)(expr))
#else
#define DEBUGASSERT(expr) \
	((expr) ? (void)0 : badassert(#expr, __FILE__, __LINE__, __func__))
#endif

/*
 * Bit flags for DEBUG()
 */
#define DB_LOCORE      0x0001
#define DB_SYSCALL     0x0002
#define DB_INTERRUPT   0x0004
#define DB_DEVICE      0x0008
#define DB_THREADS     0x0010
#define DB_VM          0x0020
#define DB_EXEC        0x0040
#define DB_VFS         0x0080
#define DB_SEMFS       0x0100
#define DB_SFS         0x0200
#define DB_NET         0x0400
#define DB_NETFS       0x0800
#define DB_KMALLOC     0x1000

extern uint32_t dbflags;

/*
 * DEBUG() is for conditionally printing debug messages to the console.
 *
 * The idea is that you put lots of lines of the form
 *
 *      DEBUG(DB_VM, "VM free pages: %u\n", free_pages);
 *
 * throughout the kernel; then you can toggle whether these messages
 * are printed or not at runtime by setting the value of dbflags with
 * the debugger.
 *
 * Unfortunately, as of this writing, there are only a very few such
 * messages actually present in the system yet. Feel free to add more.
 *
 * DEBUG is a varargs macro. These were added to the language in C99.
 */
#define DEBUG(d, ...) ((dbflags & (d)) ? kprintf(__VA_ARGS__) : 0)

/*
 * Random number generator, using the random device.
 *
 * random() returns a number between 0 and randmax() inclusive.
 */
#define RANDOM_MAX (randmax())
uint32_t randmax(void);
uint32_t random(void);

/*
 * Kernel heap memory allocation. Like malloc/free.
 * If out of memory, kmalloc returns NULL.
 *
 * kheap_nextgeneration, dump, and dumpall do nothing unless heap
 * labeling (for leak detection) in kmalloc.c (q.v.) is enabled.
 */
void *kmalloc(size_t size);
void kfree(void *ptr);
void kheap_printstats(void);
void kheap_nextgeneration(void);
void kheap_dump(void);
void kheap_dumpall(void);

/*
 * C string functions.
 *
 * kstrdup is like strdup, but calls kmalloc instead of malloc.
 * If out of memory, it returns NULL.
 */
size_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
char *kstrdup(const char *str);
char *strchr(const char *searched, int searchfor);
char *strrchr(const char *searched, int searchfor);
char *strtok_r(char *buf, const char *seps, char **context);

void *memcpy(void *dest, const void *src, size_t len);
void *memmove(void *dest, const void *src, size_t len);
void *memset(void *block, int ch, size_t len);
void bzero(void *ptr, size_t len);
int atoi(const char *str);

int snprintf(char *buf, size_t maxlen, const char *fmt, ...) __PF(3,4);

const char *strerror(int errcode);

/*
 * Low-level console access.
 */
void putch(int ch);
int getch(void);
void beep(void);

/*
 * Higher-level console output.
 *
 * kprintf is like printf, only in the kernel.
 * panic prepends the string "panic: " to the message printed, and then
 * resets the system.
 * badassert calls panic in a way suitable for an assertion failure.
 * kgets is like gets, only with a buffer size argument.
 *
 * kprintf_bootstrap sets up a lock for kprintf and should be called
 * during boot once malloc is available and before any additional
 * threads are created.
 */
int kprintf(const char *format, ...) __PF(1,2);
__DEAD void panic(const char *format, ...) __PF(1,2);
__DEAD void badassert(const char *expr, const char *file,
		      int line, const char *func);

void kgets(char *buf, size_t maxbuflen);

void kprintf_bootstrap(void);

/*
 * Other miscellaneous stuff
 */

#define DIVROUNDUP(a,b) (((a)+(b)-1)/(b))
#define ROUNDUP(a,b)    (DIVROUNDUP(a,b)*b)


#endif /* _LIB_H_ */
