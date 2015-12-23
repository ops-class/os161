/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2009, 2013
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

#ifndef FREEMAP_H
#define FREEMAP_H

/*
 * The freemap module accumulates information about the free block
 * bitmap as other checks are made, and then uses that information
 * to check and correct it.
 */

#include <stdint.h>

typedef enum {
	B_SUPERBLOCK,	/* Block that is the superblock */
	B_FREEMAPBLOCK,	/* Block used by free-block bitmap */
	B_INODE,	/* Block that is an inode */
	B_IBLOCK,	/* Indirect (or doubly-indirect etc.) block */
	B_DIRDATA,	/* Data block of a directory */
	B_DATA,		/* Data block */
	B_PASTEND,	/* Block off the end of the fs */
} blockusage_t;

/* Call this after loading the superblock but before doing any checks. */
void freemap_setup(void);

/* Call this to note that a block has been found in use. */
void freemap_blockinuse(uint32_t block, blockusage_t how, uint32_t howdesc);

/* Note that a block has been found where it should be dropped. */
void freemap_blockfree(uint32_t block);

/* Call this after all checks that call freemap_block{inuse,free}. */
void freemap_check(void);

/* Return the number of blocks in use. Valid after freemap_check(). */
unsigned long freemap_blocksused(void);


#endif /* FREEMAP_H */
