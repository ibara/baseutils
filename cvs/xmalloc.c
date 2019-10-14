/* $OpenBSD: xmalloc.c,v 1.14 2019/06/28 05:44:09 deraadt Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Versions of malloc and friends that check their results, and never return
 * failure (they call fatal if they encounter an error).
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "xmalloc.h"

void *
xmalloc(size_t size)
{
	void *ptr;

	if (size == 0)
		fatal("xmalloc: zero size");
	ptr = malloc(size);
	if (ptr == NULL)
		fatal("xmalloc: allocating %zu bytes: %s",
		    size, strerror(errno));
	return ptr;
}

void *
xcalloc(size_t nmemb, size_t size)
{
	void *ptr;

	if (size == 0 || nmemb == 0)
		fatal("xcalloc: zero size");
	ptr = calloc(nmemb, size);
	if (ptr == NULL)
		fatal("xcalloc: allocating %zu * %zu bytes: %s",
		    nmemb, size, strerror(errno));
	return ptr;
}

void *
xreallocarray(void *ptr, size_t nmemb, size_t size)
{
	void *new_ptr;

	if (nmemb == 0 || size == 0)
		fatal("xreallocarray: zero size");
	new_ptr = reallocarray(ptr, nmemb, size);
	if (new_ptr == NULL)
		fatal("xreallocarray: allocating %zu * %zu bytes: %s",
		    nmemb, size, strerror(errno));
	return new_ptr;
}

char *
xstrdup(const char *str)
{
	char *cp;

	if ((cp = strdup(str)) == NULL)
		fatal("xstrdup: %s", strerror(errno));
	return cp;
}

int
xasprintf(char **ret, const char *fmt, ...)
{
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = vasprintf(ret, fmt, ap);
	va_end(ap);

	if (i == -1)
		fatal("xasprintf: %s", strerror(errno));

	return i;
}

int
xsnprintf(char *str, size_t len, const char *fmt, ...)
{
	va_list ap;
	int i;

	if (len > INT_MAX)
		fatal("xsnprintf: len > INT_MAX");

	va_start(ap, fmt);
	i = vsnprintf(str, len, fmt, ap);
	va_end(ap);

	if (i < 0 || i >= (int)len)
		fatal("xsnprintf: overflow");

	return i;
}
