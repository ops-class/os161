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

#include <sys/types.h>	/* for CHAR_BIT */
#include <limits.h>	/* also for CHAR_BIT */
#include <stdint.h>
#include <assert.h>
#include <err.h>

#include "compat.h"
#include <kern/sfs.h>

#include "utils.h"
#include "sfs.h"
#include "sb.h"
#include "freemap.h"
#include "main.h"

static struct sfs_superblock sb;

/*
 * Load the superblock.
 */
void
sb_load(void)
{
	sfs_readsb(SFS_SUPER_BLOCK, &sb);
	if (sb.sb_magic != SFS_MAGIC) {
		errx(EXIT_FATAL, "Not an sfs filesystem");
	}

	assert(sb.sb_nblocks > 0);
	assert(SFS_FREEMAPBLOCKS(sb.sb_nblocks) > 0);
}

/*
 * Validate the superblock.
 */
void
sb_check(void)
{
	int schanged=0;

	/*
	 * FUTURE: should we check sb.sb_nblocks against diskblocks()?
	 */

	/* Check the superblock fields */

	if (checknullstring(sb.sb_volname, sizeof(sb.sb_volname))) {
		warnx("Volume name not null-terminated (fixed)");
		setbadness(EXIT_RECOV);
		schanged = 1;
	}
	if (checkbadstring(sb.sb_volname)) {
		warnx("Volume name contains illegal characters (fixed)");
		setbadness(EXIT_RECOV);
		schanged = 1;
	}
	if (checkzeroed(sb.reserved, sizeof(sb.reserved))) {
		warnx("Reserved section of superblock not zeroed (fixed)");
		setbadness(EXIT_RECOV);
		schanged = 1;
	}

	/* Write the superblock back if necessary */
	if (schanged) {
		sfs_writesb(SFS_SUPER_BLOCK, &sb);
	}
}

/*
 * Return the total number of blocks in the volume.
 */
uint32_t
sb_totalblocks(void)
{
	return sb.sb_nblocks;
}

/*
 * Return the number of freemap blocks.
 * (this function probably ought to go away)
 */
uint32_t
sb_freemapblocks(void)
{
	return SFS_FREEMAPBLOCKS(sb.sb_nblocks);
}

/*
 * Return the volume name.
 */
const char *
sb_volname(void)
{
	return sb.sb_volname;
}
