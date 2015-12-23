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

#ifndef _STDARG_H_
#define _STDARG_H_


/* Get __PF() for declaring printf-like functions. */
#include <cdefs.h>

/*
 * As of gcc 3.0, the stdarg declarations can be made machine-
 * independent because gcc abstracts the implementations away for
 * us. However, they went and changed __builtin_stdarg_start to
 * __builtin_va_start sometime between gcc 4.1 and 4.8 (not sure
 * when) so we need to check that.
 */

#ifdef __GNUC__
typedef __va_list va_list;

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#define va_start(ap, fmt)  __builtin_stdarg_start(ap, fmt)
#else
#define va_start(ap, fmt)  __builtin_va_start(ap, fmt)
#endif
#define va_arg(ap,t)       __builtin_va_arg(ap, t)
#define va_copy(ap1, ap2)  __builtin_va_copy(ap1, ap2)
#define va_end(ap)         __builtin_va_end(ap)
#endif

/*
 * The v... versions of printf functions in <lib.h>. This is not
 * really the best place for them... but if we put them in <lib.h>
 * we'd need to either include this file there, put these defs there,
 * or split the definition of va_list into another header file, none
 * of which seems entirely desirable.
 */
void vkprintf(const char *fmt, va_list ap) __PF(1,0);
int vsnprintf(char *buf, size_t maxlen, const char *fmt, va_list ap) __PF(3,0);

/*
 * The printf driver function (shared with libc).
 * Does v...printf, passing the output data piecemeal to the function
 * supplied. The "clientdata" argument is passed through to the function.
 * The strings passed to the function might not be null-terminated; the
 * supplied length should be used explicitly.
 */
int __vprintf(void (*func)(void *clientdata, const char *str, size_t len),
              void *clientdata, const char *format, va_list ap) __PF(3,0);


#endif /* _STDARG_H_ */
