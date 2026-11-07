#include <libvcc/builtins.h>

#include "harness.h"
#include "wide.h"

/*
 * The reference is the shift done one bit at a time, applied as many times as
 * the count asks for. It is not the helper written again: the helper picks a
 * word-aligned path for a count of 64 or more, and this never leaves the
 * one-bit case, so a wrong branch in the helper has nowhere to hide.
 */

static libvcc_u128 shl1(libvcc_u128 v)
{
	libvcc_u128 r;
	unsigned long long carry;

	carry = v.lo >> 63;
	r.lo = v.lo << 1;
	r.hi = (v.hi << 1) | carry;
	return r;
}

static libvcc_u128 lshr1(libvcc_u128 v)
{
	libvcc_u128 r;
	unsigned long long carry;

	carry = v.hi << 63;
	r.lo = (v.lo >> 1) | carry;
	r.hi = v.hi >> 1;
	return r;
}

static libvcc_u128 ashr1(libvcc_u128 v)
{
	libvcc_u128 r;
	unsigned long long carry, fill;

	carry = v.hi << 63;
	fill = (v.hi & 0x8000000000000000ULL) ? 0x8000000000000000ULL : 0ULL;
	r.lo = (v.lo >> 1) | carry;
	r.hi = (v.hi >> 1) | fill;
	return r;
}

static libvcc_u128 repeat(libvcc_u128 (*one)(libvcc_u128), libvcc_u128 v,
                          int b)
{
	while (b > 0) {
		v = one(v);
		b--;
	}
	return v;
}

void libvcc_test_shift128(void)
{
	unsigned long long state;
	libvcc_u128 a, want;
	int b, i;

	state = 0x452821e638d01377ULL;
	for (b = 0; b < 128; b++) {
		for (i = 0; i < 8; i++) {
			a = libvcc_test_wide_random(&state);
			CHECK_WIDE(__ashlti3(a, b), repeat(shl1, a, b));
			CHECK_WIDE(__lshrti3(a, b), repeat(lshr1, a, b));
			CHECK_WIDE(__ashrti3(a, b), repeat(ashr1, a, b));
		}
	}

	/* The bits that only a two-word shift can put in the wrong place. */
	a = WIDE(1ULL, 0ULL);
	CHECK_WIDE(__ashlti3(a, 63), WIDE(0x8000000000000000ULL, 0ULL));
	CHECK_WIDE(__ashlti3(a, 64), WIDE(0ULL, 1ULL));
	CHECK_WIDE(__ashlti3(a, 127), WIDE(0ULL, 0x8000000000000000ULL));
	CHECK_WIDE(__lshrti3(WIDE(0ULL, 1ULL), 64), WIDE(1ULL, 0ULL));
	CHECK_WIDE(__lshrti3(WIDE(0ULL, 0x8000000000000000ULL), 127),
	           WIDE(1ULL, 0ULL));
	CHECK_WIDE(__ashrti3(WIDE(0ULL, 0x8000000000000000ULL), 127),
	           WIDE(0xffffffffffffffffULL, 0xffffffffffffffffULL));

	/* A count of zero is the identity, for every shape of value. */
	CHECK_WIDE(__ashlti3(WIDE(0x0123456789abcdefULL, 0xfedcba9876543210ULL), 0),
	           WIDE(0x0123456789abcdefULL, 0xfedcba9876543210ULL));
	CHECK_WIDE(__lshrti3(WIDE(0x0123456789abcdefULL, 0xfedcba9876543210ULL), 0),
	           WIDE(0x0123456789abcdefULL, 0xfedcba9876543210ULL));
	CHECK_WIDE(__ashrti3(WIDE(0x0123456789abcdefULL, 0xfedcba9876543210ULL), 0),
	           WIDE(0x0123456789abcdefULL, 0xfedcba9876543210ULL));

	/* The fill, with and without a sign bit set above the count. */
	CHECK_WIDE(__ashrti3(WIDE(0ULL, 0x8000000000000000ULL), 1),
	           WIDE(0ULL, 0xc000000000000000ULL));
	CHECK_WIDE(__ashrti3(WIDE(0ULL, 0x8000000000000000ULL), 64),
	           WIDE(0x8000000000000000ULL, 0xffffffffffffffffULL));
	CHECK_WIDE(__ashrti3(WIDE(0ULL, 0x8000000000000000ULL), 65),
	           WIDE(0xc000000000000000ULL, 0xffffffffffffffffULL));
	CHECK_WIDE(__ashrti3(WIDE(1ULL, 0ULL), 64), WIDE(0ULL, 0ULL));
	CHECK_WIDE(__ashrti3(WIDE(0ULL, 0x7fffffffffffffffULL), 63),
	           WIDE(0xfffffffffffffffeULL, 0ULL));

	/* A count past the width answers what libgcc answers. */
	want = WIDE(0ULL, 0ULL);
	CHECK_WIDE(__ashlti3(WIDE(1ULL, 2ULL), 128), want);
	CHECK_WIDE(__lshrti3(WIDE(1ULL, 2ULL), 128), want);
	CHECK_WIDE(__ashrti3(WIDE(0ULL, 0ULL), 200), want);
	CHECK_WIDE(__ashrti3(WIDE(1ULL, 0x8000000000000000ULL), 128),
	           WIDE(0xffffffffffffffffULL, 0xffffffffffffffffULL));
}
