/*
 * Copyright (c) 2013
 *	The President and Fellows of Harvard College.
 *      Written by David A. Holland.
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

#include <string.h>
#include <assert.h>
#include <err.h>

#include "name.h"

#define MAXNAMES 32

static const char *const names[MAXNAMES] = {
	"allspice",
	"anise",
	"basil",
	"cardamom",
	"cayenne",
	"cilantro",
	"cinnamon",
	"cloves",
	"coriander",
	"cumin",
	"dill",
	"fennel",
	"fenugreek",
	"galangal",
	"ginger",
	"horseradish",
	"lemongrass",
	"licorice",
	"mace",
	"marjoram",
	"mustard",
	"nutmeg",
	"oregano",
	"parsley",
	"paprika",
	"pepper",
	"saffron",
	"sage",
	"rosemary",
	"thyme",
	"turmeric",
	"wasabi",
};

const char *
name_get(unsigned name)
{
	assert(name < MAXNAMES);
	return names[name];
}

unsigned
name_find(const char *namestr)
{
	unsigned i;

	for (i=0; i<MAXNAMES; i++) {
		if (!strcmp(namestr, names[i])) {
			return i;
		}
	}
	errx(1, "Encountered unknown/unexpected name %s", namestr);
	return 0;
}
