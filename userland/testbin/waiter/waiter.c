/*
 * waiter.c
 * 
 * Just sits there without doing anything. We use the read system call just to
 * provide a way to wait. Intended to test our ability to detect stuck
 * processes in userspace.
 */

#include <unistd.h>
#include <err.h>

int
main(void)
{
	char ch=0;
	int len;

	while (ch!='q') {
		len = read(STDIN_FILENO, &ch, 1);
		if (len < 0) {
			err(1, "stdin: read");
		}
		if (len==0) {
			/* EOF */
			break;
		}
	}
	return 0;
}
