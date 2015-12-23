/*
 * Copyright (c) 1997, 1998, 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
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
 * Guts of printf.
 *
 * This file is used in both libc and the kernel and needs to work in both
 * contexts. This makes a few things a bit awkward.
 *
 * This is a slightly simplified version of the real-life printf
 * originally used in the VINO kernel.
 */

#ifdef _KERNEL
#include <types.h>
#include <lib.h>
#define assert KASSERT
#else

#include <sys/types.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#endif

#include <stdarg.h>


/*
 * Do we want to support "long long" types with %lld?
 *
 * Using 64-bit types with gcc causes gcc to emit calls to functions
 * like __moddi3 and __divdi3. These need to be provided at link time,
 * which can be a hassle; this switch is provided to help avoid
 * needing them.
 */
#define USE_LONGLONG

/*
 * Define a type that holds the longest signed integer we intend to support.
 */
#ifdef USE_LONGLONG
#define INTTYPE  long long
#else
#define INTTYPE  long
#endif


/*
 * Space for a long long in base 8, plus a NUL, plus one
 * character extra for slop.
 *
 * CHAR_BIT is the number of bits in a char; thus sizeof(long long)*CHAR_BIT
 * is the number of bits in a long long. Each octal digit prints 3 bits.
 * Values printed in larger bases will be shorter strings.
 */
#define NUMBER_BUF_SIZE ((sizeof(INTTYPE) * CHAR_BIT) / 3 + 2)

/*
 * Structure holding the state for printf.
 */
typedef struct {
	/* Callback for sending printed string data */
	void (*sendfunc)(void *clientdata, const char *str, size_t len);
	void *clientdata;

	/* The varargs argument pointer */
	va_list ap;

	/* Total count of characters printed */
	int charcount;

	/* Flag that's true if we are currently looking in a %-format */
	int in_pct;

	/* Size of the integer argument to retrieve */
	enum {
		INTSZ,
		LONGSZ,
#ifdef USE_LONGLONG
		LLONGSZ,
#endif
		SIZETSZ,
	} size;

	/* The value of the integer argument retrieved */
	unsigned INTTYPE num;

	/* Sign of the integer argument (0 = positive; -1 = negative) */
	int sign;

	/* Field width (number of spaces) */
	int spacing;

	/* Flag: align to left in field instead of right */
	int rightspc;

	/* Character to pad to field size with (space or 0) */
	int fillchar;

	/* Number base to print the integer argument in (8, 10, 16) */
	int base;

	/* Flag: if set, print 0x before hex and 0 before octal numbers */
	int baseprefix;

	/* Flag: alternative output format selected with %#... */
	int altformat;
} PF;

/*
 * Send some text onward to the output.
 *
 * We count the total length we send out so we can return it from __vprintf,
 * since that's what most printf-like functions want to return.
 */
static
void
__pf_print(PF *pf, const char *txt, size_t len)
{
	pf->sendfunc(pf->clientdata, txt, len);
	pf->charcount += len;
}

/*
 * Reset the state for the next %-field.
 */
static
void
__pf_endfield(PF *pf)
{
	pf->in_pct = 0;
	pf->size = INTSZ;
	pf->num = 0;
	pf->sign = 0;
	pf->spacing = 0;
	pf->rightspc = 0;
	pf->fillchar = ' ';
	pf->base = 0;
	pf->baseprefix = 0;
	pf->altformat = 0;
}

/*
 * Process modifier chars (between the % and the type specifier)
 *    #           use "alternate display format"
 *    -           left align in field instead of right align
 *    l           value is long (ll = long long)
 *    z           value is size_t
 *    0-9         field width
 *    leading 0   pad with zeros instead of spaces
 */
static
void
__pf_modifier(PF *pf, int ch)
{
	switch (ch) {
	case '#':
		pf->altformat = 1;
		break;
	case '-':
		pf->rightspc = 1;
		break;
	case 'l':
		if (pf->size==LONGSZ) {
#ifdef USE_LONGLONG
			pf->size = LLONGSZ;
#endif
		}
		else {
			pf->size = LONGSZ;
		}
		break;
	case 'z':
		pf->size = SIZETSZ;
		break;
	case '0':
		if (pf->spacing>0) {
			/*
			 * Already seen some digits; this is part of the
			 * field size.
			 */
			pf->spacing = pf->spacing*10;
		}
		else {
			/*
			 * Leading zero; set the padding character to 0.
			 */
			pf->fillchar = '0';
		}
		break;
	default:
		/*
		 * Invalid characters should be filtered out by a
		 * higher-level function, so if this assert goes off
		 * it's our fault.
		 */
		assert(ch>'0' && ch<='9');

		/*
		 * Got a digit; accumulate the field size.
		 */
		pf->spacing = pf->spacing*10 + (ch-'0');
		break;
	}
}

/*
 * Retrieve a numeric argument from the argument list and store it
 * in pf->num, according to the size recorded in pf->size and using
 * the numeric type specified by ch.
 */
static
void
__pf_getnum(PF *pf, int ch)
{
	if (ch=='p') {
		/*
		 * Pointer.
		 *
		 * uintptr_t is a C99 standard type that's an unsigned
		 * integer the same size as a pointer.
		 */
		pf->num = (uintptr_t) va_arg(pf->ap, void *);
	}
	else if (ch=='d') {
		/* signed integer */
		INTTYPE signednum=0;
		switch (pf->size) {
		case INTSZ:
			/* %d */
			signednum = va_arg(pf->ap, int);
			break;
		case LONGSZ:
			/* %ld */
			signednum = va_arg(pf->ap, long);
			break;
#ifdef USE_LONGLONG
		case LLONGSZ:
			/* %lld */
			signednum = va_arg(pf->ap, long long);
			break;
#endif
		case SIZETSZ:
			/* %zd */
			signednum = va_arg(pf->ap, ssize_t);
			break;
		}

		/*
		 * Check for negative numbers.
		 */
		if (signednum < 0) {
			pf->sign = -1;
			pf->num = -signednum;
		}
		else {
			pf->num = signednum;
		}
	}
	else {
		/* unsigned integer */
		switch (pf->size) {
		case INTSZ:
			/* %u (or %o, %x) */
			pf->num = va_arg(pf->ap, unsigned int);
			break;
		case LONGSZ:
			/* %lu (or %lo, %lx) */
			pf->num = va_arg(pf->ap, unsigned long);
			break;
#ifdef USE_LONGLONG
		case LLONGSZ:
			/* %llu, %llo, %llx */
			pf->num = va_arg(pf->ap, unsigned long long);
			break;
#endif
		case SIZETSZ:
			/* %zu, %zo, %zx */
			pf->num = va_arg(pf->ap, size_t);
			break;
		}
	}
}

/*
 * Set the printing base based on the numeric type specified in ch.
 *     %o     octal
 *     %d,%u  decimal
 *     %x     hex
 *     %p     pointer (print as hex)
 *
 * If the "alternate format" was requested, or always for pointers,
 * note to print the C prefix for the type.
 */
static
void
__pf_setbase(PF *pf, int ch)
{
	switch (ch) {
	case 'd':
	case 'u':
		pf->base = 10;
		break;
	case 'x':
	case 'p':
		pf->base = 16;
		break;
	case 'o':
		pf->base = 8;
		break;
	}
	if (pf->altformat || ch=='p') {
		pf->baseprefix = 1;
	}
}

/*
 * Function to print "spc" instances of the fill character.
 */
static
void
__pf_fill(PF *pf, int spc)
{
	char f = pf->fillchar;
	int i;
	for (i=0; i<spc; i++) {
		__pf_print(pf, &f, 1);
	}
}

/*
 * General printing function. Prints the string "stuff".
 * The two prefixes (in practice one is a type prefix, such as "0x",
 * and the other is the sign) get printed *after* space padding but
 * *before* zero padding, if padding is on the left.
 */
static
void
__pf_printstuff(PF *pf,
		const char *prefix, const char *prefix2,
		const char *stuff)
{
	/* Total length to print. */
	int len = strlen(prefix)+strlen(prefix2)+strlen(stuff);

	/* Get field width and compute amount of padding in "spc". */
	int spc = pf->spacing;
	if (spc > len) {
		spc -= len;
	}
	else {
		spc = 0;
	}

	/* If padding on left and the fill char is not 0, pad first. */
	if (spc > 0 && pf->rightspc==0 && pf->fillchar!='0') {
		__pf_fill(pf, spc);
	}

	/* Print the prefixes. */
	__pf_print(pf, prefix, strlen(prefix));
	__pf_print(pf, prefix2, strlen(prefix2));

	/* If padding on left and the fill char *is* 0, pad here. */
	if (spc > 0 && pf->rightspc==0 && pf->fillchar=='0') {
		__pf_fill(pf, spc);
	}

	/* Print the actual string. */
	__pf_print(pf, stuff, strlen(stuff));

	/* If padding on the right, pad afterwards. */
	if (spc > 0 && pf->rightspc!=0) {
		__pf_fill(pf, spc);
	}
}

/*
 * Function to convert a number to ascii and then print it.
 *
 * Works from right to left in a buffer of NUMBER_BUF_SIZE bytes.
 * NUMBER_BUF_SIZE is set so that the longest number string we can
 * generate (a long long printed in octal) will fit. See above.
 */
static
void
__pf_printnum(PF *pf)
{
	/* Digits to print with. */
	const char *const digits = "0123456789abcdef";

	char buf[NUMBER_BUF_SIZE];   /* Accumulation buffer for string. */
	char *x;                     /* Current pointer into buf. */
	unsigned INTTYPE xnum;       /* Current value to print. */
	const char *bprefix;         /* Base prefix (0, 0x, or nothing) */
	const char *sprefix;         /* Sign prefix (- or nothing) */

	/* Start in the last slot of the buffer. */
	x = buf+sizeof(buf)-1;

	/* Insert null terminator. */
	*x-- = 0;

	/* Initialize value. */
	xnum = pf->num;

	/*
	 * Convert a single digit.
	 * Do this loop at least once - that way 0 prints as 0 and not "".
	 */
	do {
		/*
		 * Get the digit character for the least significant
		 * part of xnum.
		 */
		*x = digits[xnum % pf->base];

		/*
		 * Back up the pointer to point to the next space to the left.
		 */
		x--;

		/*
		 * Drop the value of the digit we just printed from xnum.
		 */
		xnum = xnum / pf->base;

		/*
		 * If xnum hits 0 there's no more number left.
		 */
	} while (xnum > 0);

	/*
	 * x points to the *next* slot in the buffer to use.
	 * However, we're done printing the number. So it's pointing
	 * one slot *before* the start of the actual number text.
	 * So advance it by one so it actually points at the number.
	 */
	x++;

	/*
	 * If a base prefix was requested, select it.
	 */
	if (pf->baseprefix && pf->base==16) {
		bprefix = "0x";
	}
	else if (pf->baseprefix && pf->base==8) {
		bprefix = "0";
	}
	else {
		bprefix = "";
	}

	/*
	 * Choose the sign prefix.
	 */
	sprefix = pf->sign ? "-" : "";

	/*
	 * Now actually print the string we just generated.
	 */
	__pf_printstuff(pf, sprefix, bprefix, x);
}

/*
 * Process a single character out of the format string.
 */
static
void
__pf_send(PF *pf, int ch)
{
	/* Cannot get NULs here. */
	assert(ch!=0);

	if (pf->in_pct==0 && ch!='%') {
		/*
		 * Not currently in a format, and not a %. Just send
		 * the character on through.
		 */
		char c = ch;
		__pf_print(pf, &c, 1);
	}
	else if (pf->in_pct==0) {
		/*
		 * Not in a format, but got a %. Start a format.
		 */
		pf->in_pct = 1;
	}
	else if (strchr("#-lz0123456789", ch)) {
		/*
		 * These are the modifier characters we recognize.
		 * (These are the characters between the % and the type.)
		 */
		__pf_modifier(pf, ch);
	}
	else if (strchr("doupx", ch)) {
		/*
		 * Integer types.
		 * Fetch the number, set the base, print it, then
		 * reset for the next format.
		 */
		__pf_getnum(pf, ch);
		__pf_setbase(pf, ch);
		__pf_printnum(pf);
		__pf_endfield(pf);
	}
	else if (ch=='s') {
		/*
		 * Print a string.
		 */
		const char *str = va_arg(pf->ap, const char *);
		if (str==NULL) {
			str = "(null)";
		}
		__pf_printstuff(pf, "", "", str);
		__pf_endfield(pf);
	}
	else {
		/*
		 * %%, %c, or illegal character.
		 * Illegal characters are printed like %%.
		 * for example, %5k prints "    k".
		 */
		char x[2];
		if (ch=='c') {
			x[0] = va_arg(pf->ap, int);
		}
		else {
			x[0] = ch;
		}
		x[1] = 0;
		__pf_printstuff(pf, "", "", x);
		__pf_endfield(pf);
	}
}

/*
 * Do a whole printf session.
 * Create and initialize a printf state object,
 * then send it each character from the format string.
 */
int
__vprintf(void (*func)(void *clientdata, const char *str, size_t len),
	  void *clientdata, const char *format, va_list ap)
{
	PF pf;
	int i;

	pf.sendfunc = func;
	pf.clientdata = clientdata;
#ifdef va_copy
	va_copy(pf.ap, ap);
#else
	pf.ap = ap;
#endif
	pf.charcount = 0;
	__pf_endfield(&pf);

	for (i=0; format[i]; i++) {
		__pf_send(&pf, format[i]);
	}

	return pf.charcount;
}
