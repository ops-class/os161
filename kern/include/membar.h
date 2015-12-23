/*
 * Copyright (c) 2013
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

#ifndef _MEMBAR_H_
#define _MEMBAR_H_

/*
 * Memory barriers. These create ordering barriers in CPU memory
 * accesses as actually issued by the CPU to the cache and memory
 * system. Because superscalar CPUs can execute many instructions at
 * once, they can potentially be retired in a different order from
 * what's written in your code. Normally this doesn't matter, but
 * sometimes it does (e.g. when writing to device registers) and in
 * those cases you need to insert memory barrier instructions to
 * create ordering guarantees.
 *
 * membar_load_load creates an ordering barrier between preceding
 * loads (from memory to registers) and subsequent loads, but has
 * (potentially) no effect on stores. This is what some people call a
 * "load fence".
 *
 * membar_store_store creates an ordering barrier between preceding
 * stores (from registers to memory) and subsequent stores, but has
 * (potentially) no effect on loads. This is what some people call a
 * "store" or "write fence".
 *
 * membar_store_any creates an ordering barrier between preceding
 * stores and subsequent stores *and* loads. Preceding loads may be
 * delayed past the barrier. This is the behavior needed for
 * operations comparable to spinlock_acquire().
 *
 * membar_any_store creates an ordering barrier between preceding
 * loads and stores and subsequent stores. Following loads may be
 * executed before the barrier. This is the behavior needed for
 * operations comparable to spinlock_release().
 *
 * membar_any_any creates a full ordering barrier, between preceding
 * loads and stores and following loads and stores.
 *
 * In OS/161 we assume that the spinlock operations include any memory
 * barrier instructions they require. (On many CPUs the synchronized/
 * locked instructions used to implement spinlocks are themselves
 * implicit memory barriers.) You do not need to use membar_store_any
 * and membar_any_store unless rolling your own lock-like objects,
 * using atomic operations, implementing lock-free data structures, or
 * talking to hardware devices.
 *
 * There is a lot of FUD about memory barriers circulating on the
 * internet. Please ask your course staff if you have questions or
 * concerns.
 */

/* Inlining support - for making sure an out-of-line copy gets built */
#ifndef MEMBAR_INLINE
#define MEMBAR_INLINE INLINE
#endif

MEMBAR_INLINE void membar_load_load(void);
MEMBAR_INLINE void membar_store_store(void);
MEMBAR_INLINE void membar_store_any(void);
MEMBAR_INLINE void membar_any_store(void);
MEMBAR_INLINE void membar_any_any(void);

/* Get the implementation. */
#include <machine/membar.h>

#endif /* _MEMBAR_H_ */
