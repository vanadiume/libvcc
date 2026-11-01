#include <libvcc/builtins.h>

/*
 * The counts are the 64-bit ones put together, because that is what they are:
 * a leading zero count is the high word's, or 64 plus the low word's when the
 * high word is all zero, and a bit can only be in one word at a time.
 *
 * Working from the 64-bit helpers means these four answer zero the way those
 * answer zero, which is the width: 128 for a clz or a ctz, 0 for a popcount or
 * a parity. That case is the reason each one is still written out rather than
 * left to the caller to guard.
 */

int __clzti2(libvcc_u128 x)
{
	if (x.hi != 0)
		return __clzdi2(x.hi);
	return 64 + __clzdi2(x.lo);
}

int __ctzti2(libvcc_u128 x)
{
	if (x.lo != 0)
		return __ctzdi2(x.lo);
	return 64 + __ctzdi2(x.hi);
}

int __popcountti2(libvcc_u128 x)
{
	return __popcountdi2(x.lo) + __popcountdi2(x.hi);
}

int __parityti2(libvcc_u128 x)
{
	return __paritydi2(x.lo) ^ __paritydi2(x.hi);
}
