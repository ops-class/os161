/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David A. Holland.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <cdefs.h>
#include <lib.h>

#define ARRAYS_CHECKED

#ifdef ARRAYS_CHECKED
#define ARRAYASSERT KASSERT
#else
#define ARRAYASSERT(x) ((void)(x))
#endif

#ifndef ARRAYINLINE
#define ARRAYINLINE INLINE
#endif

/*
 * Base array type (resizeable array of void pointers) and operations.
 *
 * create - allocate an array.
 * destroy - destroy an allocated array.
 * init - initialize an array in space externally allocated.
 * cleanup - clean up an array in space externally allocated.
 * num - return number of elements in array.
 * get - return element no. INDEX.
 * set - set element no. INDEX to VAL.
 * preallocate - allocate space without changing size; may fail and
 *       return error.
 * setsize - change size to NUM elements; may fail and return error.
 * add - append VAL to end of array; return its index in INDEX_RET if
 *       INDEX_RET isn't null; may fail and return error.
 * remove - excise entry INDEX and slide following entries down to
 *       close the resulting gap.
 *
 * Note that expanding an array with setsize doesn't initialize the new
 * elements. (Usually the caller is about to store into them anyway.)
 */

struct array {
	void **v;
	unsigned num, max;
};

struct array *array_create(void);
void array_destroy(struct array *);
void array_init(struct array *);
void array_cleanup(struct array *);
ARRAYINLINE unsigned array_num(const struct array *);
ARRAYINLINE void *array_get(const struct array *, unsigned index);
ARRAYINLINE void array_set(const struct array *, unsigned index, void *val);
int array_preallocate(struct array *, unsigned num);
int array_setsize(struct array *, unsigned num);
ARRAYINLINE int array_add(struct array *, void *val, unsigned *index_ret);
void array_remove(struct array *, unsigned index);

/*
 * Inlining for base operations
 */

ARRAYINLINE unsigned
array_num(const struct array *a)
{
	return a->num;
}

ARRAYINLINE void *
array_get(const struct array *a, unsigned index)
{
	ARRAYASSERT(index < a->num);
	return a->v[index];
}

ARRAYINLINE void
array_set(const struct array *a, unsigned index, void *val)
{
	ARRAYASSERT(index < a->num);
	a->v[index] = val;
}

ARRAYINLINE int
array_add(struct array *a, void *val, unsigned *index_ret)
{
	unsigned index;
	int ret;

	index = a->num;
	ret = array_setsize(a, index+1);
	if (ret) {
		return ret;
	}
	a->v[index] = val;
	if (index_ret != NULL) {
		*index_ret = index;
	}
	return 0;
}

/*
 * Bits for declaring and defining typed arrays.
 *
 * Usage:
 *
 * DECLARRAY_BYTYPE(foo, bar) declares "struct foo", which is
 * an array of pointers to "bar", plus the operations on it.
 *
 * DECLARRAY(foo) is equivalent to DECLARRAY_BYTYPE(fooarray, struct foo).
 *
 * DEFARRAY_BYTYPE and DEFARRAY are the same as DECLARRAY except that
 * they define the operations, and both take an extra argument INLINE.
 * For C99 this should be INLINE in header files and empty in the
 * master source file, the same as the usage of ARRAYINLINE above and
 * in array.c.
 *
 * Example usage in e.g. item.h of some game:
 *
 * DECLARRAY_BYTYPE(stringarray, char);
 * DECLARRAY(potion);
 * DECLARRAY(sword);
 *
 * #ifndef ITEMINLINE
 * #define ITEMINLINE INLINE
 * #endif
 *
 * DEFARRAY_BYTYPE(stringarray, char, ITEMINLINE);
 * DEFARRAY(potion, ITEMINLINE);
 * DEFARRAY(sword, ITEMINLINE);
 *
 * Then item.c would do "#define ITEMINLINE" before including item.h.
 *
 * This creates types "struct stringarray", "struct potionarray",
 * and "struct swordarray", with operations such as "swordarray_num".
 *
 * The operations on typed arrays are the same as the operations on
 * the base array, except typed.
 */

#define DECLARRAY_BYTYPE(ARRAY, T, INLINE) \
	struct ARRAY {						\
		struct array arr;				\
	};							\
								\
	INLINE struct ARRAY *ARRAY##_create(void);		\
	INLINE void ARRAY##_destroy(struct ARRAY *a);		\
	INLINE void ARRAY##_init(struct ARRAY *a);		\
	INLINE void ARRAY##_cleanup(struct ARRAY *a);		\
	INLINE unsigned ARRAY##_num(const struct ARRAY *a);	\
	INLINE T *ARRAY##_get(const struct ARRAY *a, unsigned index); \
	INLINE void ARRAY##_set(struct ARRAY *a, unsigned index, T *val); \
	INLINE int ARRAY##_preallocate(struct ARRAY *a, unsigned num);	\
	INLINE int ARRAY##_setsize(struct ARRAY *a, unsigned num);	\
	INLINE int ARRAY##_add(struct ARRAY *a, T *val, unsigned *index_ret); \
	INLINE void ARRAY##_remove(struct ARRAY *a, unsigned index)

#define DEFARRAY_BYTYPE(ARRAY, T, INLINE) \
	INLINE struct ARRAY *					\
	ARRAY##_create(void)					\
	{							\
		struct ARRAY *a = kmalloc(sizeof(*a));		\
		if (a == NULL) {				\
			return NULL;				\
		}						\
		array_init(&a->arr);				\
		return a;					\
	}							\
								\
	INLINE void						\
	ARRAY##_destroy(struct ARRAY *a)			\
	{							\
		array_cleanup(&a->arr);				\
		kfree(a);					\
	}							\
								\
	INLINE void						\
	ARRAY##_init(struct ARRAY *a)				\
	{							\
		array_init(&a->arr);				\
	}							\
								\
	INLINE void						\
	ARRAY##_cleanup(struct ARRAY *a)			\
	{							\
		array_cleanup(&a->arr);				\
	}							\
								\
	INLINE unsigned						\
	ARRAY##_num(const struct ARRAY *a)			\
	{							\
		return array_num(&a->arr);			\
	}							\
								\
	INLINE T *						\
	ARRAY##_get(const struct ARRAY *a, unsigned index)	\
	{							\
		return (T *)array_get(&a->arr, index);		\
	}							\
								\
	INLINE void						\
	ARRAY##_set(struct ARRAY *a, unsigned index, T *val)	\
	{							\
		array_set(&a->arr, index, (void *)val);		\
	}							\
								\
	INLINE int						\
	ARRAY##_preallocate(struct ARRAY *a, unsigned num)	\
	{							\
		return array_preallocate(&a->arr, num);		\
	}							\
								\
	INLINE int						\
	ARRAY##_setsize(struct ARRAY *a, unsigned num)		\
	{							\
		return array_setsize(&a->arr, num);		\
	}							\
								\
	INLINE int						\
	ARRAY##_add(struct ARRAY *a, T *val, unsigned *index_ret) \
	{							\
		return array_add(&a->arr, (void *)val, index_ret); \
	}							\
								\
	INLINE void						\
	ARRAY##_remove(struct ARRAY *a, unsigned index)		\
	{							\
		array_remove(&a->arr, index);			\
	}

#define DECLARRAY(T, INLINE) DECLARRAY_BYTYPE(T##array, struct T, INLINE)
#define DEFARRAY(T, INLINE) DEFARRAY_BYTYPE(T##array, struct T, INLINE)

/*
 * This is how you declare an array of strings; it works out as
 * an array of pointers to char.
 */
DECLARRAY_BYTYPE(stringarray, char, ARRAYINLINE);
DEFARRAY_BYTYPE(stringarray, char, ARRAYINLINE);


#endif /* ARRAY_H */
