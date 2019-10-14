/*
 * Public domain
 * Bad randomness
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "openbsd.h"

u_int32_t
arc4random(void)
{

	srand(time(NULL));

	return ((u_int32_t) rand());
}

void
arc4random_buf(char *buf, size_t nbytes)
{
	size_t i;

	srand(time(NULL));

	for (i = 0; i < nbytes; i++)
		buf[i] = (unsigned char) rand();
}

u_int32_t
arc4random_uniform(u_int32_t upper_bound)
{

	return (arc4random() % upper_bound);
}
