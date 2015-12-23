/*
 * Copyright (c) 2003, 2006, 2008, 2009
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

#ifndef _CDEFS_H_
#define _CDEFS_H_

/*
 * Some miscellaneous C language definitions and related matters.
 */


/*
 * Build-time assertion. Doesn't generate any code. The error message
 * on failure is less than ideal, but you can't have everything.
 */
#define COMPILE_ASSERT(x) ((void)sizeof(struct { unsigned : ((x)?1:-1); }))


/*
 * Handy macro for the number of elements in a static array.
 */
#define ARRAYCOUNT(arr) (sizeof(arr) / sizeof((arr)[0]))


/*
 * Tell GCC how to check printf formats. Also tell it about functions
 * that don't return, as this is helpful for avoiding bogus warnings
 * about uninitialized variables.
 */
#ifdef __GNUC__
#define __PF(a,b) __attribute__((__format__(__printf__, a, b)))
#define __DEAD    __attribute__((__noreturn__))
#define __UNUSED  __attribute__((__unused__))
#else
#define __PF(a,b)
#define __DEAD
#define __UNUSED
#endif


/*
 * Material for supporting inline functions.
 *
 * A function marked inline can be handled by the compiler in three
 * ways: in addition to possibly inlining into the code for other
 * functions, the compiler can (1) generate a file-static out-of-line
 * copy of the function, (2) generate a global out-of-line copy of the
 * function, or (3) generate no out-of-line copy of the function.
 *
 * None of these alone is thoroughly satisfactory. Since an inline
 * function may or may not be inlined at the compiler's discretion, if
 * no out-of-line copy exists the build may fail at link time with
 * undefined symbols. Meanwhile, if the compiler is told to generate a
 * global out-of-line copy, it will generate one such copy for every
 * source file where the inline definition is visible; since inline
 * functions tend to appear in header files, this leads to multiply
 * defined symbols and build failure. The file-static option isn't
 * really an improvement, either: one tends to get compiler warnings
 * about inline functions that haven't been used, which for any
 * particular source file tends to be at least some of the ones that
 * have been defined. Furthermore, this method leads to one
 * out-of-line copy of the inline function per source file that uses
 * it, which not only wastes space but makes debugging painful.
 *
 * Therefore, we use the following scheme.
 *
 * In the header file containing the inline functions for the module
 * "foo", we put
 *
 *      #ifndef FOO_INLINE
 *      #define FOO_INLINE INLINE
 *      #endif
 *
 * where INLINE selects the compiler behavior that does *not* generate
 * an out-of-line version. Then we define the inline functions
 * themselves as FOO_INLINE. This allows the compiler to inline the
 * functions anywhere it sees fit with a minimum of hassles. Then,
 * when compiling foo.c, before including headers we put
 *
 *      #define FOO_INLINE  // empty
 *
 * which causes the inline functions to appear as ordinary function
 * definitions, not inline at all, when foo.c is compiled. This
 * ensures that an out-of-line definition appears, and furthermore
 * ensures that the out-of-line definition is the same as the inline
 * definition.
 *
 * The situation is complicated further because gcc is historically
 * not compliant with the C standard. In C99, "inline" means "do not
 * generate an out-of-line copy" and "extern inline" means "generate a
 * global out-of-line copy". In gcc, going back far longer than C99,
 * the meanings were reversed. This eventually changed, but varies
 * with compiler version and options. The macro __GNUC_STDC_INLINE__
 * is defined if the behavior is C99-compliant.
 *
 * (Note that inline functions that appear only within a single source
 * file can safely be declared "static inline"; to avoid whining from
 * compiler in some contexts you may also want to add __UNUSED to
 * that.)
 */
#if defined(__GNUC__) && !defined(__GNUC_STDC_INLINE__)
/* gcc's non-C99 inline semantics */
#define INLINE extern inline

#elif defined(__STDC__) && __STDC_VERSION__ >= 199901L
/* C99 */
#define INLINE inline

#else
/* something else; static inline is safest */
#define INLINE static __UNUSED inline
#endif


#endif /* _CDEFS_H_ */
