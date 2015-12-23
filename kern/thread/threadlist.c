/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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
 * Thread list functions, rather dull.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <threadlist.h>

void
threadlistnode_init(struct threadlistnode *tln, struct thread *t)
{
	DEBUGASSERT(tln != NULL);
	KASSERT(t != NULL);

	tln->tln_next = NULL;
	tln->tln_prev = NULL;
	tln->tln_self = t;
}

void
threadlistnode_cleanup(struct threadlistnode *tln)
{
	DEBUGASSERT(tln != NULL);

	KASSERT(tln->tln_next == NULL);
	KASSERT(tln->tln_prev == NULL);
	KASSERT(tln->tln_self != NULL);
}

void
threadlist_init(struct threadlist *tl)
{
	DEBUGASSERT(tl != NULL);

	tl->tl_head.tln_next = &tl->tl_tail;
	tl->tl_head.tln_prev = NULL;
	tl->tl_tail.tln_next = NULL;
	tl->tl_tail.tln_prev = &tl->tl_head;
	tl->tl_head.tln_self = NULL;
	tl->tl_tail.tln_self = NULL;
	tl->tl_count = 0;
}

void
threadlist_cleanup(struct threadlist *tl)
{
	DEBUGASSERT(tl != NULL);
	DEBUGASSERT(tl->tl_head.tln_next == &tl->tl_tail);
	DEBUGASSERT(tl->tl_head.tln_prev == NULL);
	DEBUGASSERT(tl->tl_tail.tln_next == NULL);
	DEBUGASSERT(tl->tl_tail.tln_prev == &tl->tl_head);
	DEBUGASSERT(tl->tl_head.tln_self == NULL);
	DEBUGASSERT(tl->tl_tail.tln_self == NULL);

	KASSERT(threadlist_isempty(tl));
	KASSERT(tl->tl_count == 0);

	/* nothing (else) to do */
}

bool
threadlist_isempty(struct threadlist *tl)
{
	DEBUGASSERT(tl != NULL);

	return (tl->tl_count == 0);
}

////////////////////////////////////////////////////////////
// internal

/*
 * Do insertion. Doesn't update tl_count.
 */
static
void
threadlist_insertafternode(struct threadlistnode *onlist, struct thread *t)
{
	struct threadlistnode *addee;

	addee = &t->t_listnode;

	DEBUGASSERT(addee->tln_prev == NULL);
	DEBUGASSERT(addee->tln_next == NULL);

	addee->tln_prev = onlist;
	addee->tln_next = onlist->tln_next;
	addee->tln_prev->tln_next = addee;
	addee->tln_next->tln_prev = addee;
}

/*
 * Do insertion. Doesn't update tl_count.
 */
static
void
threadlist_insertbeforenode(struct thread *t, struct threadlistnode *onlist)
{
	struct threadlistnode *addee;

	addee = &t->t_listnode;

	DEBUGASSERT(addee->tln_prev == NULL);
	DEBUGASSERT(addee->tln_next == NULL);

	addee->tln_prev = onlist->tln_prev;
	addee->tln_next = onlist;
	addee->tln_prev->tln_next = addee;
	addee->tln_next->tln_prev = addee;
}

/*
 * Do removal. Doesn't update tl_count.
 */
static
void
threadlist_removenode(struct threadlistnode *tln)
{
	DEBUGASSERT(tln != NULL);
	DEBUGASSERT(tln->tln_prev != NULL);
	DEBUGASSERT(tln->tln_next != NULL);

	tln->tln_prev->tln_next = tln->tln_next;
	tln->tln_next->tln_prev = tln->tln_prev;
	tln->tln_prev = NULL;
	tln->tln_next = NULL;
}

////////////////////////////////////////////////////////////
// public

void
threadlist_addhead(struct threadlist *tl, struct thread *t)
{
	DEBUGASSERT(tl != NULL);
	DEBUGASSERT(t != NULL);

	threadlist_insertafternode(&tl->tl_head, t);
	tl->tl_count++;
}

void
threadlist_addtail(struct threadlist *tl, struct thread *t)
{
	DEBUGASSERT(tl != NULL);
	DEBUGASSERT(t != NULL);

	threadlist_insertbeforenode(t, &tl->tl_tail);
	tl->tl_count++;
}

struct thread *
threadlist_remhead(struct threadlist *tl)
{
	struct threadlistnode *tln;

	DEBUGASSERT(tl != NULL);

	tln = tl->tl_head.tln_next;
	if (tln->tln_next == NULL) {
		/* list was empty  */
		return NULL;
	}
	threadlist_removenode(tln);
	DEBUGASSERT(tl->tl_count > 0);
	tl->tl_count--;
	return tln->tln_self;
}

struct thread *
threadlist_remtail(struct threadlist *tl)
{
	struct threadlistnode *tln;

	DEBUGASSERT(tl != NULL);

	tln = tl->tl_tail.tln_prev;
	if (tln->tln_prev == NULL) {
		/* list was empty  */
		return NULL;
	}
	threadlist_removenode(tln);
	DEBUGASSERT(tl->tl_count > 0);
	tl->tl_count--;
	return tln->tln_self;
}

void
threadlist_insertafter(struct threadlist *tl,
		       struct thread *onlist, struct thread *addee)
{
	threadlist_insertafternode(&onlist->t_listnode, addee);
	tl->tl_count++;
}

void
threadlist_insertbefore(struct threadlist *tl,
			struct thread *addee, struct thread *onlist)
{
	threadlist_insertbeforenode(addee, &onlist->t_listnode);
	tl->tl_count++;
}

void
threadlist_remove(struct threadlist *tl, struct thread *t)
{
	threadlist_removenode(&t->t_listnode);
	DEBUGASSERT(tl->tl_count > 0);
	tl->tl_count--;
}
