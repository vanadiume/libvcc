#include <libvcc/builtins.h>

#include "word.h"

/*
 * The product of two 32-bit values without a 64-bit multiply: split each one
 * into 16-bit halves and add four products that do fit in a word. On a
 * 32-bit target a 64-bit multiplication compiles to a call to __muldi3, so
 * the helper cannot be written with the operation it provides.
 *
 * The intermediate sum is named because vcc's -O1 and above mistype an
 * arithmetic expression used directly as the left operand of a shift.
 */

static unsigned long long mul32(unsigned int a, unsigned int b)
{
	unsigned int a0, a1, b0, b1;
	unsigned long long r, mid;

	a0 = a & 0xffffU;
	a1 = a >> 16;
	b0 = b & 0xffffU;
	b1 = b >> 16;
	r = (unsigned long long)(a0 * b0);
	mid = (unsigned long long)(a0 * b1) + (unsigned long long)(a1 * b0);
	r += mid << 16;
	mid = (unsigned long long)(a1 * b1);
	r += mid << 32;
	return r;
}

long long __muldi3(long long a, long long b)
{
	unsigned long long ua, ub, lo, mid;

	ua = (unsigned long long)a;
	ub = (unsigned long long)b;
	lo = mul32(LIBVCC_LO64(ua), LIBVCC_LO64(ub));
	mid = mul32(LIBVCC_LO64(ua), LIBVCC_HI64(ub)) +
	      mul32(LIBVCC_HI64(ua), LIBVCC_LO64(ub));
	return (long long)(lo + (mid << 32));
}
