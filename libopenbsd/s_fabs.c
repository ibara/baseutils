/* Public domain */

#include <stdint.h>
#include <stdlib.h>

#include "math_private.h"

#include "openbsd.h"

double
fabs(double x)
{
	int32_t hx;

	GET_HIGH_WORD(hx, x);

	hx = abs(hx);

	SET_HIGH_WORD(x, hx);

	return x;
}
