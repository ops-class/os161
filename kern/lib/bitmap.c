/*
 * Copyright (c) 2000, 2001, 2002
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

/*
 * Fixed-size array of bits. (Intended for storage management.)
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <bitmap.h>

/*
 * It would be a lot more efficient on most platforms to use uint32_t
 * or unsigned long as the base type for holding bits. But we don't,
 * because if one uses any data type more than a single byte wide,
 * bitmap data saved on disk becomes endian-dependent, which is a
 * severe nuisance.
 */
#define BITS_PER_WORD   (CHAR_BIT)
#define WORD_TYPE       unsigned char
#define WORD_ALLBITS    (0xff)

struct bitmap {
        unsigned nbits;
        WORD_TYPE *v;
};


struct bitmap *
bitmap_create(unsigned nbits)
{
        struct bitmap *b;
        unsigned words;

        words = DIVROUNDUP(nbits, BITS_PER_WORD);
        b = kmalloc(sizeof(struct bitmap));
        if (b == NULL) {
                return NULL;
        }
        b->v = kmalloc(words*sizeof(WORD_TYPE));
        if (b->v == NULL) {
                kfree(b);
                return NULL;
        }

        bzero(b->v, words*sizeof(WORD_TYPE));
        b->nbits = nbits;

        /* Mark any leftover bits at the end in use */
        if (words > nbits / BITS_PER_WORD) {
                unsigned j, ix = words-1;
                unsigned overbits = nbits - ix*BITS_PER_WORD;

                KASSERT(nbits / BITS_PER_WORD == words-1);
                KASSERT(overbits > 0 && overbits < BITS_PER_WORD);

                for (j=overbits; j<BITS_PER_WORD; j++) {
                        b->v[ix] |= ((WORD_TYPE)1 << j);
                }
        }

        return b;
}

void *
bitmap_getdata(struct bitmap *b)
{
        return b->v;
}

int
bitmap_alloc(struct bitmap *b, unsigned *index)
{
        unsigned ix;
        unsigned maxix = DIVROUNDUP(b->nbits, BITS_PER_WORD);
        unsigned offset;

        for (ix=0; ix<maxix; ix++) {
                if (b->v[ix]!=WORD_ALLBITS) {
                        for (offset = 0; offset < BITS_PER_WORD; offset++) {
                                WORD_TYPE mask = ((WORD_TYPE)1) << offset;

                                if ((b->v[ix] & mask)==0) {
                                        b->v[ix] |= mask;
                                        *index = (ix*BITS_PER_WORD)+offset;
                                        KASSERT(*index < b->nbits);
                                        return 0;
                                }
                        }
                        KASSERT(0);
                }
        }
        return ENOSPC;
}

static
inline
void
bitmap_translate(unsigned bitno, unsigned *ix, WORD_TYPE *mask)
{
        unsigned offset;
        *ix = bitno / BITS_PER_WORD;
        offset = bitno % BITS_PER_WORD;
        *mask = ((WORD_TYPE)1) << offset;
}

void
bitmap_mark(struct bitmap *b, unsigned index)
{
        unsigned ix;
        WORD_TYPE mask;

        KASSERT(index < b->nbits);
        bitmap_translate(index, &ix, &mask);

        KASSERT((b->v[ix] & mask)==0);
        b->v[ix] |= mask;
}

void
bitmap_unmark(struct bitmap *b, unsigned index)
{
        unsigned ix;
        WORD_TYPE mask;

        KASSERT(index < b->nbits);
        bitmap_translate(index, &ix, &mask);

        KASSERT((b->v[ix] & mask)!=0);
        b->v[ix] &= ~mask;
}


int
bitmap_isset(struct bitmap *b, unsigned index)
{
        unsigned ix;
        WORD_TYPE mask;

        bitmap_translate(index, &ix, &mask);
        return (b->v[ix] & mask);
}

void
bitmap_destroy(struct bitmap *b)
{
        kfree(b->v);
        kfree(b);
}
