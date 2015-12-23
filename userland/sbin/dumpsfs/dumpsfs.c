/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009, 2014
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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <err.h>

#include "support.h"
#include "kern/sfs.h"


#ifdef HOST
/*
 * OS/161 runs natively on a big-endian platform, so we can
 * conveniently use the byteswapping functions for network byte order.
 */
#include <netinet/in.h> // for arpa/inet.h
#include <arpa/inet.h>  // for ntohl
#include "hostcompat.h"
#define SWAP64(x) ntohll(x)
#define SWAP32(x) ntohl(x)
#define SWAP16(x) ntohs(x)

extern const char *hostcompat_progname;

#else

#define SWAP64(x) (x)
#define SWAP32(x) (x)
#define SWAP16(x) (x)

#endif

#include "disk.h"

#define ARRAYCOUNT(a) (sizeof(a) / sizeof((a)[0]))
#define DIVROUNDUP(a, b) (((a) + (b) - 1) / (b))

static bool dofiles, dodirs;
static bool doindirect;
static bool recurse;

////////////////////////////////////////////////////////////
// printouts

static unsigned dumppos;

static
void
dumpval(const char *desc, const char *val)
{
	size_t dlen, vlen, used;

	dlen = strlen(desc);
	vlen = strlen(val);

	printf("    ");

	printf("%s: %s", desc, val);

	used = dlen + 2 + vlen;
	for (; used < 36; used++) {
		putchar(' ');
	}

	if (dumppos % 2 == 1) {
		printf("\n");
	}
	dumppos++;
}

static
void
dumpvalf(const char *desc, const char *valf, ...)
{
	va_list ap;
	char buf[128];

	va_start(ap, valf);
	vsnprintf(buf, sizeof(buf), valf, ap);
	va_end(ap);
	dumpval(desc, buf);
}

static
void
dumplval(const char *desc, const char *lval)
{
	if (dumppos % 2 == 1) {
		printf("\n");
		dumppos++;
	}
	printf("    %s: %s\n", desc, lval);
	dumppos += 2;
}

////////////////////////////////////////////////////////////
// fs structures

static void dumpinode(uint32_t ino, const char *name);

static
uint32_t
readsb(void)
{
	struct sfs_superblock sb;

	diskread(&sb, SFS_SUPER_BLOCK);
	if (SWAP32(sb.sb_magic) != SFS_MAGIC) {
		errx(1, "Not an sfs filesystem");
	}
	return SWAP32(sb.sb_nblocks);
}

static
void
dumpsb(void)
{
	struct sfs_superblock sb;
	unsigned i;

	diskread(&sb, SFS_SUPER_BLOCK);
	sb.sb_volname[sizeof(sb.sb_volname)-1] = 0;

	printf("Superblock\n");
	printf("----------\n");
	dumpvalf("Magic", "0x%8x", SWAP32(sb.sb_magic));
	dumpvalf("Size", "%u blocks", SWAP32(sb.sb_nblocks));
	dumpvalf("Freemap size", "%u blocks",
		 SFS_FREEMAPBLOCKS(SWAP32(sb.sb_nblocks)));
	dumpvalf("Block size", "%u bytes", SFS_BLOCKSIZE);
	dumplval("Volume name", sb.sb_volname);

	for (i=0; i<ARRAYCOUNT(sb.reserved); i++) {
		if (sb.reserved[i] != 0) {
			printf("    Word %u in reserved area: 0x%x\n",
			       i, SWAP32(sb.reserved[i]));
		}
	}
	printf("\n");
}

static
void
dumpfreemap(uint32_t fsblocks)
{
	uint32_t freemapblocks = SFS_FREEMAPBLOCKS(fsblocks);
	uint32_t i, j, k, bn;
	uint8_t data[SFS_BLOCKSIZE], mask;
	char tmp[16];

	printf("Free block bitmap\n");
	printf("-----------------\n");
	for (i=0; i<freemapblocks; i++) {
		diskread(data, SFS_FREEMAP_START+i);
		printf("    Freemap block #%u in disk block %u: blocks %u - %u"
		       " (0x%x - 0x%x)\n",
		       i, SFS_FREEMAP_START+i,
		       i*SFS_BITSPERBLOCK, (i+1)*SFS_BITSPERBLOCK - 1,
		       i*SFS_BITSPERBLOCK, (i+1)*SFS_BITSPERBLOCK - 1);
		for (j=0; j<SFS_BLOCKSIZE; j++) {
			if (j % 8 == 0) {
				snprintf(tmp, sizeof(tmp), "0x%x",
					 i*SFS_BITSPERBLOCK + j*8);
				printf("%-7s ", tmp);
			}
			for (k=0; k<8; k++) {
				bn = i*SFS_BITSPERBLOCK + j*8 + k;
				mask = 1U << k;
				if (bn >= fsblocks) {
					if (data[j] & mask) {
						putchar('x');
					}
					else {
						putchar('!');
					}
				}
				else {
					if (data[j] & mask) {
						putchar('*');
					}
					else {
						putchar('.');
					}
				}
			}
			if (j % 8 == 7) {
				printf("\n");
			}
			else {
				printf(" ");
			}
		}
	}
	printf("\n");
}

static
void
dumpindirect(uint32_t block)
{
	uint32_t ib[SFS_BLOCKSIZE/sizeof(uint32_t)];
	char tmp[128];
	unsigned i;

	if (block == 0) {
		return;
	}
	printf("Indirect block %u\n", block);

	diskread(ib, block);
	for (i=0; i<ARRAYCOUNT(ib); i++) {
		if (i % 4 == 0) {
			printf("@%-3u   ", i);
		}
		snprintf(tmp, sizeof(tmp), "%u (0x%x)",
			 SWAP32(ib[i]), SWAP32(ib[i]));
		printf("  %-16s", tmp);
		if (i % 4 == 3) {
			printf("\n");
		}
	}
}

static
uint32_t
traverse_ib(uint32_t fileblock, uint32_t numblocks, uint32_t block,
	    void (*doblock)(uint32_t, uint32_t))
{
	uint32_t ib[SFS_BLOCKSIZE/sizeof(uint32_t)];
	unsigned i;

	if (block == 0) {
		memset(ib, 0, sizeof(ib));
	}
	else {
		diskread(ib, block);
	}
	for (i=0; i<ARRAYCOUNT(ib) && fileblock < numblocks; i++) {
		doblock(fileblock++, SWAP32(ib[i]));
	}
	return fileblock;
}

static
void
traverse(const struct sfs_dinode *sfi, void (*doblock)(uint32_t, uint32_t))
{
	uint32_t fileblock;
	uint32_t numblocks;
	unsigned i;

	numblocks = DIVROUNDUP(SWAP32(sfi->sfi_size), SFS_BLOCKSIZE);

	fileblock = 0;
	for (i=0; i<SFS_NDIRECT && fileblock < numblocks; i++) {
		doblock(fileblock++, SWAP32(sfi->sfi_direct[i]));
	}
	if (fileblock < numblocks) {
		fileblock = traverse_ib(fileblock, numblocks,
					SWAP32(sfi->sfi_indirect), doblock);
	}
	assert(fileblock == numblocks);
}

static
void
dumpdirblock(uint32_t fileblock, uint32_t diskblock)
{
	struct sfs_direntry sds[SFS_BLOCKSIZE/sizeof(struct sfs_direntry)];
	int nsds = SFS_BLOCKSIZE/sizeof(struct sfs_direntry);
	int i;

	(void)fileblock;
	if (diskblock == 0) {
		printf("    [block %u - empty]\n", diskblock);
		return;
	}
	diskread(&sds, diskblock);

	printf("    [block %u]\n", diskblock);
	for (i=0; i<nsds; i++) {
		uint32_t ino = SWAP32(sds[i].sfd_ino);
		if (ino==SFS_NOINO) {
			printf("        [free entry]\n");
		}
		else {
			sds[i].sfd_name[SFS_NAMELEN-1] = 0; /* just in case */
			printf("        %u %s\n", ino, sds[i].sfd_name);
		}
	}
}

static
void
dumpdir(uint32_t ino, const struct sfs_dinode *sfi)
{
	int nentries;

	nentries = SWAP32(sfi->sfi_size) / sizeof(struct sfs_direntry);
	if (SWAP32(sfi->sfi_size) % sizeof(struct sfs_direntry) != 0) {
		warnx("Warning: dir size is not a multiple of dir entry size");
	}
	printf("Directory contents for inode %u: %d entries\n", ino, nentries);
	traverse(sfi, dumpdirblock);
}

static
void
recursedirblock(uint32_t fileblock, uint32_t diskblock)
{
	struct sfs_direntry sds[SFS_BLOCKSIZE/sizeof(struct sfs_direntry)];
	int nsds = SFS_BLOCKSIZE/sizeof(struct sfs_direntry);
	int i;

	(void)fileblock;
	if (diskblock == 0) {
		return;
	}
	diskread(&sds, diskblock);

	for (i=0; i<nsds; i++) {
		uint32_t ino = SWAP32(sds[i].sfd_ino);
		if (ino==SFS_NOINO) {
			continue;
		}
		sds[i].sfd_name[SFS_NAMELEN-1] = 0; /* just in case */
		dumpinode(ino, sds[i].sfd_name);
	}
}

static
void
recursedir(uint32_t ino, const struct sfs_dinode *sfi)
{
	int nentries;

	nentries = SWAP32(sfi->sfi_size) / sizeof(struct sfs_direntry);
	printf("Reading files in directory %u: %d entries\n", ino, nentries);
	traverse(sfi, recursedirblock);
	printf("Done with directory %u\n", ino);
}

static
void dumpfileblock(uint32_t fileblock, uint32_t diskblock)
{
	uint8_t data[SFS_BLOCKSIZE];
	unsigned i, j;
	char tmp[128];

	if (diskblock == 0) {
		printf("    0x%6x  [sparse]\n", fileblock * SFS_BLOCKSIZE);
		return;
	}

	diskread(data, diskblock);
	for (i=0; i<SFS_BLOCKSIZE; i++) {
		if (i % 16 == 0) {
			snprintf(tmp, sizeof(tmp), "0x%x",
				 fileblock * SFS_BLOCKSIZE + i);
			printf("%8s", tmp);
		}
		if (i % 8 == 0) {
			printf("  ");
		}
		else {
			printf(" ");
		}
		printf("%02x", data[i]);
		if (i % 16 == 15) {
			printf("  ");
			for (j = i-15; j<=i; j++) {
				if (data[j] < 32 || data[j] > 126) {
					putchar('.');
				}
				else {
					putchar(data[j]);
				}
			}
			printf("\n");
		}
	}
}

static
void
dumpfile(uint32_t ino, const struct sfs_dinode *sfi)
{
	printf("File contents for inode %u:\n", ino);
	traverse(sfi, dumpfileblock);
}

static
void
dumpinode(uint32_t ino, const char *name)
{
	struct sfs_dinode sfi;
	const char *typename;
	char tmp[128];
	unsigned i;

	diskread(&sfi, ino);

	printf("Inode %u", ino);
	if (name != NULL) {
		printf(" (%s)", name);
	}
	printf("\n");
	printf("--------------\n");

	switch (SWAP16(sfi.sfi_type)) {
	    case SFS_TYPE_FILE: typename = "regular file"; break;
	    case SFS_TYPE_DIR: typename = "directory"; break;
	    default: typename = "invalid"; break;
	}
	dumpvalf("Type", "%u (%s)", SWAP16(sfi.sfi_type), typename);
	dumpvalf("Size", "%u", SWAP32(sfi.sfi_size));
	dumpvalf("Link count", "%u", SWAP16(sfi.sfi_linkcount));
	printf("\n");

        printf("    Direct blocks:\n");
        for (i=0; i<SFS_NDIRECT; i++) {
		if (i % 4 == 0) {
			printf("@%-2u    ", i);
		}
		/*
		 * Assume the disk size might be > 64K sectors (which
		 * would be 32M) but is < 1024K sectors (512M) so we
		 * need up to 5 hex digits for a block number. And
		 * assume it's actually < 1 million sectors so we need
		 * only up to 6 decimal digits. The complete block
		 * number print then needs up to 16 digits.
		 */
		snprintf(tmp, sizeof(tmp), "%u (0x%x)",
			 SWAP32(sfi.sfi_direct[i]), SWAP32(sfi.sfi_direct[i]));
		printf("  %-16s", tmp);
		if (i % 4 == 3) {
			printf("\n");
		}
	}
	if (i % 4 != 0) {
		printf("\n");
	}
	printf("    Indirect block: %u (0x%x)\n",
	       SWAP32(sfi.sfi_indirect), SWAP32(sfi.sfi_indirect));
	for (i=0; i<ARRAYCOUNT(sfi.sfi_waste); i++) {
		if (sfi.sfi_waste[i] != 0) {
			printf("    Word %u in waste area: 0x%x\n",
			       i, SWAP32(sfi.sfi_waste[i]));
		}
	}

	if (doindirect) {
		dumpindirect(SWAP32(sfi.sfi_indirect));
	}

	if (SWAP16(sfi.sfi_type) == SFS_TYPE_DIR && dodirs) {
		dumpdir(ino, &sfi);
	}
	if (SWAP16(sfi.sfi_type) == SFS_TYPE_FILE && dofiles) {
		dumpfile(ino, &sfi);
	}
	if (SWAP16(sfi.sfi_type) == SFS_TYPE_DIR && recurse) {
		recursedir(ino, &sfi);
	}
}

////////////////////////////////////////////////////////////
// main

static
void
usage(void)
{
	warnx("Usage: dumpsfs [options] device/diskfile");
	warnx("   -s: dump superblock");
	warnx("   -b: dump free block bitmap");
	warnx("   -i ino: dump specified inode");
	warnx("   -I: dump indirect blocks");
	warnx("   -f: dump file contents");
	warnx("   -d: dump directory contents");
	warnx("   -r: recurse into directory contents");
	warnx("   -a: equivalent to -sbdfr -i 1");
	errx(1, "   Default is -i 1");
}

int
main(int argc, char **argv)
{
	bool dosb = false;
	bool dofreemap = false;
	uint32_t dumpino = 0;
	const char *dumpdisk = NULL;

	int i, j;
	uint32_t nblocks;

#ifdef HOST
	/* Don't do this; it frobs the tty and you can't pipe to less */
	/*hostcompat_init(argc, argv);*/
	hostcompat_progname = argv[0];
#endif

	for (i=1; i<argc; i++) {
		if (argv[i][0] == '-') {
			for (j=1; argv[i][j]; j++) {
				switch (argv[i][j]) {
				    case 's': dosb = true; break;
				    case 'b': dofreemap = true; break;
				    case 'i':
					if (argv[i][j+1] == 0) {
						dumpino = atoi(argv[++i]);
					}
					else {
						dumpino = atoi(argv[i]+j+1);
						j = strlen(argv[i]);
					}
					/* XXX ugly */
					goto nextarg;
				    case 'I': doindirect = true; break;
				    case 'f': dofiles = true; break;
				    case 'd': dodirs = true; break;
				    case 'r': recurse = true; break;
				    case 'a':
					dosb = true;
					dofreemap = true;
					if (dumpino == 0) {
						dumpino = SFS_ROOTDIR_INO;
					}
					doindirect = true;
					dofiles = true;
					dodirs = true;
					recurse = true;
					break;
				    default:
					usage();
					break;
				}
			}
		}
		else {
			if (dumpdisk != NULL) {
				usage();
			}
			dumpdisk = argv[i];
		}
	 nextarg:
		;
	}
	if (dumpdisk == NULL) {
		usage();
	}

	if (!dosb && !dofreemap && dumpino == 0) {
		dumpino = SFS_ROOTDIR_INO;
	}

	opendisk(dumpdisk);
	nblocks = readsb();

	if (dosb) {
		dumpsb();
	}
	if (dofreemap) {
		dumpfreemap(nblocks);
	}
	if (dumpino != 0) {
		dumpinode(dumpino, NULL);
	}

	closedisk();

	return 0;
}
