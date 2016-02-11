#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/secret.h>
#include <spinlock.h>

int rwtest(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt1 unimplemented\n");
	success(FAIL, SECRET, "rwt1");

	return 0;
}
