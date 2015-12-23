/*
 * Copyright (c) 2000, 2001
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

#ifndef _BITMAP_H_
#define _BITMAP_H_

/*
 * Fixed-size array of bits. (Intended for storage management.)
 *
 * Functions:
 *     bitmap_create  - allocate a new bitmap object.
 *                      Returns NULL on error.
 *     bitmap_getdata - return pointer to raw bit data (for I/O).
 *     bitmap_alloc   - locate a cleared bit, set it, and return its index.
 *     bitmap_mark    - set a clear bit by its index.
 *     bitmap_unmark  - clear a set bit by its index.
 *     bitmap_isset   - return whether a particular bit is set or not.
 *     bitmap_destroy - destroy bitmap.
 */


struct bitmap;  /* Opaque. */

struct bitmap *bitmap_create(unsigned nbits);
void          *bitmap_getdata(struct bitmap *);
int            bitmap_alloc(struct bitmap *, unsigned *index);
void           bitmap_mark(struct bitmap *, unsigned index);
void           bitmap_unmark(struct bitmap *, unsigned index);
int            bitmap_isset(struct bitmap *, unsigned index);
void           bitmap_destroy(struct bitmap *);


#endif /* _BITMAP_H_ */
