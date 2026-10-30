#include <libvcc/builtins.h>

#include "word.h"

/*
 * The shifts are one word at a time. On a 64-bit machine the compiler folds
 * each one back into a single instruction; on a 32-bit machine these are the
 * functions a shift compiles to, so they cannot be written with a 64-bit
 * shift and expect to come back.
 *
 * The count is the ABI's: 0 through 63. Anything else is not tested and not
 * promised.
 *
 * Each half is computed into a named temporary before the two are combined.
 * vcc's -O1 and above mistype an arithmetic expression used directly as an
 * operand of |, and a temporary is the shape it reads; see AGENTS.md.
 */

long long __ashldi3(long long a, int b)
{
	unsigned int hi, lo, h, l;

	if (b == 0)
		return a;
	hi = LIBVCC_HI64(a);
	lo = LIBVCC_LO64(a);
	if (b >= 32) {
		hi = lo << (b - 32);
		lo = 0;
	} else {
		h = lo >> (32 - b);
		l = hi << b;
		hi = l | h;
		lo = lo << b;
	}
	return (long long)LIBVCC_PACK64(hi, lo);
}

long long __lshrdi3(long long a, int b)
{
	unsigned int hi, lo, h, l;

	if (b == 0)
		return a;
	hi = LIBVCC_HI64(a);
	lo = LIBVCC_LO64(a);
	if (b >= 32) {
		lo = hi >> (b - 32);
		hi = 0;
	} else {
		h = lo >> b;
		l = hi << (32 - b);
		lo = h | l;
		hi = hi >> b;
	}
	return (long long)LIBVCC_PACK64(hi, lo);
}

/*
 * The fill is written out instead of shifting a negative int, whose right
 * shift is implementation-defined in C89. The compilers agree on it, but a
 * helper whose answer depends on an implementation-defined operation is one
 * more thing to measure per compiler.
 */

long long __ashrdi3(long long a, int b)
{
	unsigned int hi, lo, fill, h, l;

	if (b == 0)
		return a;
	hi = LIBVCC_HI64(a);
	lo = LIBVCC_LO64(a);
	fill = (hi & 0x80000000U) ? 0xffffffffU : 0U;
	if (b == 32) {
		lo = hi;
		hi = fill;
	} else if (b > 32) {
		h = hi >> (b - 32);
		l = fill << (64 - b);
		lo = h | l;
		hi = fill;
	} else {
		h = lo >> b;
		l = hi << (32 - b);
		lo = h | l;
		h = hi >> b;
		l = fill << (32 - b);
		hi = h | l;
	}
	return (long long)LIBVCC_PACK64(hi, lo);
}
