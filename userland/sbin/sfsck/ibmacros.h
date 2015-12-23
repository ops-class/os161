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

#ifndef IBMACROS_H
#define IBMACROS_H

/*
 * Indirect block access macros
 *
 * These are macros for working with the direct and indirect block
 * pointers in the inode. The scheme here supports a range of possible
 * configurations, because sometimes adding large file support to SFS
 * is part of an assignment; there is and should be no obligation to
 * pick any particular layout, and we'd like sfsck to build and run
 * seamlessly provided that the values declared in kern/sfs.h are
 * correct.
 *
 * SFS_NDIRECT, SFS_NINDIRECT, SFS_NDINDIRECT, and SFS_NTINDIRECT
 * should always be defined. If zero, no corresponding field is
 * assumed to exist in the inode. If one, the field is assumed to
 * be a single value and not an array. If greater than one, the
 * field is assumed to be an array.
 */

#ifndef SFS_NDIRECT
#error "SFS_NDIRECT not defined"
#endif
#ifndef SFS_NINDIRECT
#error "SFS_NINDIRECT not defined"
#endif
#ifndef SFS_NDINDIRECT
#error "SFS_NDINDIRECT not defined"
#endif
#ifndef SFS_NTINDIRECT
#error "SFS_NTINDIRECT not defined"
#endif

/*
 * For x in D, I, II, III (direct, indirect, 2x/3x indirect) we
 * provide:
 *
 *    NUM_x	the number of blocks of this type
 *    GET_x	retrieve the i'th block of this type
 *    SET_x	lvalue for the i'th block of this type
 *    RANGE_x	size of the block range mapped with one block of this type
 *    INOMAX_x	maximum block number mapped by using this type in the inode
 *
 * It is important that the accessor macros (SET_x/GET_x) not refer to
 * a nonexistent field of the inode in the case where there are zero
 * blocks of that type, as that will lead to compile failure. Hence the
 * lengthy definitions. So far I haven't thought any useful cpp hackery
 * to make them more concise.
 */

/* numbers */

#define NUM_D   		SFS_NDIRECT
#define NUM_I			SFS_NINDIRECT
#define NUM_II			SFS_NDINDIRECT
#define NUM_III			SFS_NTINDIRECT

/* blocks */

#if NUM_D == 0
#define GET_D(sfi, i)		GET0_x(sfi, sfi_direct, i)
#define SET_D(sfi, i)		SET0_x(sfi, sfi_direct, i)
#elif NUM_D == 1
#define GET_D(sfi, i)		GET1_x(sfi, sfi_direct, i)
#define SET_D(sfi, i)		SET1_x(sfi, sfi_direct, i)
#else
#define GET_D(sfi, i)		GETN_x(sfi, sfi_direct, i)
#define SET_D(sfi, i)		SETN_x(sfi, sfi_direct, i)
#endif

#if NUM_I == 0
#define GET_I(sfi, i)		GET0_x(sfi, sfi_indirect, i)
#define SET_I(sfi, i)		SET0_x(sfi, sfi_indirect, i)
#elif NUM_I == 1
#define GET_I(sfi, i)		GET1_x(sfi, sfi_indirect, i)
#define SET_I(sfi, i)		SET1_x(sfi, sfi_indirect, i)
#else
#define GET_I(sfi, i)		GETN_x(sfi, sfi_indirect, i)
#define SET_I(sfi, i)		SETN_x(sfi, sfi_indirect, i)
#endif

#if NUM_II == 0
#define GET_II(sfi, i)		GET0_x(sfi, sfi_dindirect, i)
#define SET_II(sfi, i)		SET0_x(sfi, sfi_dindirect, i)
#elif NUM_II == 1
#define GET_II(sfi, i)		GET1_x(sfi, sfi_dindirect, i)
#define SET_II(sfi, i)		SET1_x(sfi, sfi_dindirect, i)
#else
#define GET_II(sfi, i)		GETN_x(sfi, sfi_dindirect, i)
#define SET_II(sfi, i)		SETN_x(sfi, sfi_dindirect, i)
#endif

#if NUM_III == 0
#define GET_III(sfi, i)		GET0_x(sfi, sfi_tindirect, i)
#define SET_III(sfi, i)		SET0_x(sfi, sfi_tindirect, i)
#elif NUM_III == 1
#define GET_III(sfi, i)		GET1_x(sfi, sfi_tindirect, i)
#define SET_III(sfi, i)		SET1_x(sfi, sfi_tindirect, i)
#else
#define GET_III(sfi, i)		GETN_x(sfi, sfi_tindirect, i)
#define SET_III(sfi, i)		SETN_x(sfi, sfi_tindirect, i)
#endif

/* the generic forms of the block macros */

#define GET0_x(sfi, field, i)	((void)(i), (void)(sfi), 0)
#define GET1_x(sfi, field, i)	((void)(i), (sfi)->field)
#define GETN_x(sfi, field, i)	((sfi)->field[(i)])

#define SET0_x(sfi, field, i)	(*((void)(i), (void)(sfi), (uint32_t *)NULL))
#define SET1_x(sfi, field, i)	(*((void)(i), &(sfi)->field))
#define SETN_x(sfi, field, i)	((sfi)->field[(i)])

/* region sizes */

#define RANGE_D		1
#define RANGE_I		(RANGE_D * SFS_DBPERIDB)
#define RANGE_II	(RANGE_I * SFS_DBPERIDB)
#define RANGE_III	(RANGE_II * SFS_DBPERIDB)

/* max blocks */

#define INOMAX_D 	NUM_D
#define INOMAX_I 	(INOMAX_D + SFS_DBPERIDB * NUM_I)
#define INOMAX_II	(INOMAX_I + SFS_DBPERIDB * NUM_II)
#define INOMAX_III	(INOMAX_II + SFS_DBPERIDB * NUM_III)


#endif /* IBMACROS_H */
