/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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
 * The VFS-interface operations that can write to the fs are:
 *
 *    sync
 *    fsync
 *    write
 *    reclaim
 *    truncate
 *    creat
 *    symlink (not supported by default and which we are going to ignore)
 *    mkdir
 *    link
 *    remove
 *    rmdir
 *    rename
 *
 * The sync operations are not interesting by themselves. Also,
 * because the VFS-level unmount operation works by first syncing and
 * then doing an unmount (which by default at least asserts nothing
 * further needs to be written out) there is nothing to be gained by
 * combining sync with write operations and seeing if the behavior is
 * different: if sync doesn't produce a clean fs, unmount won't
 * either, and vice versa.
 *
 * This means there are basically the following families of cases to
 * consider:
 *    - writing to files in various ways and patterns
 *    - truncating files
 *    - directory ops that create things
 *    - directory ops that remove things (various combinations with reclaim)
 *    - link and rename
 */

/*
 * writing
 */

void wl_createwrite(const char *size);
void wl_rewrite(const char *size);
void wl_randupdate(const char *size);
void wl_truncwrite(const char *size);
void wl_makehole(const char *size);
void wl_fillhole(const char *size);
void wl_truncfill(const char *size);
void wl_append(const char *size);

/*
 * truncating
 */
void wl_trunczero(const char *size);
void wl_trunconeblock(const char *size);
void wl_truncsmallersize(const char *size);
void wl_trunclargersize(const char *size);
void wl_appendandtrunczero(const char *size);
void wl_appendandtruncpartly(const char *size);

/*
 * creating
 */
void wl_mkfile(void);
void wl_mkdir(void);
void wl_mkmanyfile(void);
void wl_mkmanydir(void);
void wl_mktree(void);
void wl_mkrandtree(const char *seed);

/*
 * deleting
 */
void wl_rmfile(void);
void wl_rmdir(void);
void wl_rmfiledelayed(void);
void wl_rmfiledelayedappend(void);
void wl_rmdirdelayed(void);
void wl_rmmanyfile(void);
void wl_rmmanyfiledelayed(void);
void wl_rmmanyfiledelayedandappend(void);
void wl_rmmanydir(void);
void wl_rmmanydirdelayed(void);
void wl_rmtree(void);
void wl_rmrandtree(const char *seed);

/*
 * link
 */
void wl_linkfile(void);
void wl_linkmanyfile(void);
void wl_unlinkfile(void);
void wl_unlinkmanyfile(void);
void wl_linkunlinkfile(void);

/*
 * rename
 */
void wl_renamefile(void);
void wl_renamedir(void);
void wl_renamesubtree(void);
void wl_renamexdfile(void);
void wl_renamexddir(void);
void wl_renamexdsubtree(void);
void wl_renamemanyfile(void);
void wl_renamemanydir(void);
void wl_renamemanysubtree(void);

/*
 * Combo ops
 */
void wl_copyandrename(void);
void wl_untar(void);
void wl_compile(void);
void wl_cvsupdate(void);

/*
 * Randomized op sequences
 */
void wl_writefileseq(const char *seed);
void wl_writetruncseq(const char *seed);
void wl_mkrmseq(const char *seed);
void wl_linkunlinkseq(const char *seed);
void wl_renameseq(const char *seed);
void wl_diropseq(const char *seed);
void wl_genseq(const char *seed);
