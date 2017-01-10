/*
 * Copyright (c) 2013
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

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <threadlist.h>
#include <test.h>

#define NUMNAMES 7
static const char *const names[NUMNAMES] = {
	"Aillard",
	"Aldaran",
	"Alton",
	"Ardais",
	"Elhalyn",
	"Hastur",
	"Ridenow",
};

static struct thread *fakethreads[NUMNAMES];

////////////////////////////////////////////////////////////
// fakethread

#define FAKE_MAGIC ((void *)0xbaabaa)

/*
 * Create a dummy struct thread that we can put on lists for testing.
 */
static
struct thread *
fakethread_create(const char *name)
{
	struct thread *t;

	t = kmalloc(sizeof(*t));
	if (t == NULL) {
		panic("threadlisttest: Out of memory\n");
	}
	/* ignore most of the fields, zero everything for tidiness */
	bzero(t, sizeof(*t));
	strcpy(t->t_name, name);
	t->t_stack = FAKE_MAGIC;
	threadlistnode_init(&t->t_listnode, t);
	return t;
}

/*
 * Destroy a fake thread.
 */
static
void
fakethread_destroy(struct thread *t)
{
	KASSERT(t->t_stack == FAKE_MAGIC);
	threadlistnode_cleanup(&t->t_listnode);
	kfree(t);
}

////////////////////////////////////////////////////////////
// support stuff

static
void
check_order(struct threadlist *tl, bool rev)
{
	const char string0[] = "...";
	const char stringN[] = "~~~";

	struct thread *t;
	const char *first = rev ? stringN : string0;
	const char *last = rev ? string0 : stringN;
	const char *prev;
	int cmp;

	prev = first;
	THREADLIST_FORALL(t, *tl) {
		cmp = strcmp(prev, t->t_name);
		KASSERT(rev ? (cmp > 0) : (cmp < 0));
		prev = t->t_name;
	}
	cmp = strcmp(prev, last);
	KASSERT(rev ? (cmp > 0) : (cmp < 0));
}

////////////////////////////////////////////////////////////
// tests

static
void
threadlisttest_a(void)
{
	struct threadlist tl;

	threadlist_init(&tl);
	KASSERT(threadlist_isempty(&tl));
	threadlist_cleanup(&tl);
}

static
void
threadlisttest_b(void)
{
	struct threadlist tl;
	struct thread *t;

	threadlist_init(&tl);

	threadlist_addhead(&tl, fakethreads[0]);
	check_order(&tl, false);
	check_order(&tl, true);
	KASSERT(tl.tl_count == 1);
	t = threadlist_remhead(&tl);
	KASSERT(tl.tl_count == 0);
	KASSERT(t == fakethreads[0]);

	threadlist_addtail(&tl, fakethreads[0]);
	check_order(&tl, false);
	check_order(&tl, true);
	KASSERT(tl.tl_count == 1);
	t = threadlist_remtail(&tl);
	KASSERT(tl.tl_count == 0);
	KASSERT(t == fakethreads[0]);

	threadlist_cleanup(&tl);
}

static
void
threadlisttest_c(void)
{
	struct threadlist tl;
	struct thread *t;

	threadlist_init(&tl);

	threadlist_addhead(&tl, fakethreads[0]);
	threadlist_addhead(&tl, fakethreads[1]);
	KASSERT(tl.tl_count == 2);

	check_order(&tl, true);

	t = threadlist_remhead(&tl);
	KASSERT(t == fakethreads[1]);
	t = threadlist_remhead(&tl);
	KASSERT(t == fakethreads[0]);
	KASSERT(tl.tl_count == 0);

	threadlist_addtail(&tl, fakethreads[0]);
	threadlist_addtail(&tl, fakethreads[1]);
	KASSERT(tl.tl_count == 2);

	check_order(&tl, false);

	t = threadlist_remtail(&tl);
	KASSERT(t == fakethreads[1]);
	t = threadlist_remtail(&tl);
	KASSERT(t == fakethreads[0]);
	KASSERT(tl.tl_count == 0);

	threadlist_cleanup(&tl);
}

static
void
threadlisttest_d(void)
{
	struct threadlist tl;
	struct thread *t;

	threadlist_init(&tl);

	threadlist_addhead(&tl, fakethreads[0]);
	threadlist_addtail(&tl, fakethreads[1]);
	KASSERT(tl.tl_count == 2);

	check_order(&tl, false);

	t = threadlist_remhead(&tl);
	KASSERT(t == fakethreads[0]);
	t = threadlist_remtail(&tl);
	KASSERT(t == fakethreads[1]);
	KASSERT(tl.tl_count == 0);

	threadlist_addhead(&tl, fakethreads[0]);
	threadlist_addtail(&tl, fakethreads[1]);
	KASSERT(tl.tl_count == 2);

	check_order(&tl, false);

	t = threadlist_remtail(&tl);
	KASSERT(t == fakethreads[1]);
	t = threadlist_remtail(&tl);
	KASSERT(t == fakethreads[0]);
	KASSERT(tl.tl_count == 0);

	threadlist_cleanup(&tl);
}

static
void
threadlisttest_e(void)
{
	struct threadlist tl;
	struct thread *t;
	unsigned i;

	threadlist_init(&tl);

	threadlist_addhead(&tl, fakethreads[1]);
	threadlist_addtail(&tl, fakethreads[3]);
	KASSERT(tl.tl_count == 2);
	check_order(&tl, false);

	threadlist_insertafter(&tl, fakethreads[3], fakethreads[4]);
	KASSERT(tl.tl_count == 3);
	check_order(&tl, false);

	threadlist_insertbefore(&tl, fakethreads[0], fakethreads[1]);
	KASSERT(tl.tl_count == 4);
	check_order(&tl, false);

	threadlist_insertafter(&tl, fakethreads[1], fakethreads[2]);
	KASSERT(tl.tl_count == 5);
	check_order(&tl, false);

	KASSERT(fakethreads[4]->t_listnode.tln_prev->tln_self ==
		fakethreads[3]);
	KASSERT(fakethreads[3]->t_listnode.tln_prev->tln_self ==
		fakethreads[2]);
	KASSERT(fakethreads[2]->t_listnode.tln_prev->tln_self ==
		fakethreads[1]);
	KASSERT(fakethreads[1]->t_listnode.tln_prev->tln_self ==
		fakethreads[0]);

	for (i=0; i<5; i++) {
		t = threadlist_remhead(&tl);
		KASSERT(t == fakethreads[i]);
	}
	KASSERT(tl.tl_count == 0);

	threadlist_cleanup(&tl);
}

static
void
threadlisttest_f(void)
{
	struct threadlist tl;
	struct thread *t;
	unsigned i;

	threadlist_init(&tl);

	for (i=0; i<NUMNAMES; i++) {
		threadlist_addtail(&tl, fakethreads[i]);
	}
	KASSERT(tl.tl_count == NUMNAMES);

	i=0;
	THREADLIST_FORALL(t, tl) {
		KASSERT(t == fakethreads[i]);
		i++;
	}
	KASSERT(i == NUMNAMES);

	i=0;
	THREADLIST_FORALL_REV(t, tl) {
		KASSERT(t == fakethreads[NUMNAMES - i - 1]);
		i++;
	}
	KASSERT(i == NUMNAMES);

	for (i=0; i<NUMNAMES; i++) {
		t = threadlist_remhead(&tl);
		KASSERT(t == fakethreads[i]);
	}
	KASSERT(tl.tl_count == 0);
}

////////////////////////////////////////////////////////////
// external interface

int
threadlisttest(int nargs, char **args)
{
	unsigned i;

	(void)nargs;
	(void)args;

	kprintf("Testing threadlists...\n");

	for (i=0; i<NUMNAMES; i++) {
		fakethreads[i] = fakethread_create(names[i]);
	}

	threadlisttest_a();
	threadlisttest_b();
	threadlisttest_c();
	threadlisttest_d();
	threadlisttest_e();
	threadlisttest_f();

	for (i=0; i<NUMNAMES; i++) {
		fakethread_destroy(fakethreads[i]);
		fakethreads[i] = NULL;
	}

	kprintf("Done.\n");
	return 0;
}
