/*
 * This fake implementation of pledge(2) is bad.
 * Your OS should implement a real pledge(2).
 * Public domain, because this can't be copyrightable.
 */

#include "openbsd.h"

#ifndef __OpenBSD__
int
pledge(const char *promises, const char *execpromises)
{

	return 0;
}
#endif
