#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <kern/secret.h>

#ifdef HOST
#include "hostcompat.h"
#endif

/* printf variant that is quiet during automated testing */
int
tprintf(const char *fmt, ...)
{
	int chars;
	va_list ap;

	if (strcmp(SECRET, "") != 0) {
		return 0;
	}
	
	va_start(ap, fmt);
	chars = vprintf(fmt, ap);
	va_end(ap);

	return chars;
}

/* printf variant that is loud during automated testing */
int
nprintf(const char *fmt, ...)
{
	int chars;
	va_list ap;

	if (strcmp(SECRET, "") == 0) {
		return 0;
	}
	
	va_start(ap, fmt);
	chars = vprintf(fmt, ap);
	va_end(ap);

	return chars;
}

/* printf variant that prepends the kernel secret */
int
printsf(const char *fmt, ...)
{
	int chars;
	va_list ap;

	if (strcmp(SECRET, "") != 0) {
		printf("%s: ", SECRET);
	}
	va_start(ap, fmt);
	chars = vprintf(fmt, ap);
	va_end(ap);

	return chars;
}
