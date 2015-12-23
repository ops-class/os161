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

#ifndef INODE_H
#define INODE_H

/*
 * The inode module accumulates non-local information about files and
 * directories as other checks are made, and then updates inodes
 * accordingly after the other checks are done.
 */

/* Add an inode. Returns 1 if we've seen this inode before. */
int inode_add(uint32_t ino, int type);

/* Sort the inode table for faster lookup once all inode_add() done. */
void inode_sorttable(void);

/*
 * Remember that we've seen a particular directory. Returns nonzero if
 * we've seen this directory before, which means the directory is
 * crosslinked. Requires inode_sorttable() first.
 */
int inode_visitdir(uint32_t ino);

/*
 * Count a link to a regular file. (Not called for directories.)
 * Requires inode_sorttable() first.
 */
void inode_addlink(uint32_t ino);

/*
 * Correct the link counts of regular files, once all inode_addlink()
 * done.
 */
void inode_adjust_filelinks(void);


#endif /* INODE_H */
