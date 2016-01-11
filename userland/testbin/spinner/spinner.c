/*
 * spinner.c
 * 
 * Spins as hard as it can, forking multiple processes as needed. Intended to
 * test our ability to detect stuck processes in userspace.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>

static
void
spin(void)
{
	volatile int i;
	
	for (i=0; i <= 1000; i++) {
		if (i == 1000) {
			i = 0;
		}
	}
}

int
main(int argc, char **argv)
{
	int i, count, pid;

	if (argc != 2) {
		errx(1, "Usage: spinner <count>");
	}
	count = atoi(argv[1]);

	for (i = 1; i < count; i++) {
		pid = fork();
		if (pid != 0) {
			spin();
		}
	}
	spin();
	errx(2, "spinner: spin returned");

	// 09 Jan 2015 : GWA : Shouldn't get here.
	return 0;
}
