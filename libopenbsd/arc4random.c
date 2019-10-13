/*
 * Public domain
 * Bad randomness
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "openbsd.h"

void
arc4random_buf(char *buf, size_t nbytes)
{
	size_t i;

	srand(time(NULL));

	for (i = 0; i < nbytes; i++)
		buf[i] = (unsigned char) rand();
}
