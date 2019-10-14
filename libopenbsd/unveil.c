/*
 * This fake implementation of pledge(2) is bad.
 * Your OS should implement a real pledge(2).
 * Public domain, because this can't be copyrightable.
 */

#include "openbsd.h"

#ifndef __OpenBSD__
int
unveil(const char *path, const char *permissions)
{

	return 0;
}
#endif
