#include <libvcc/compiler.h>

#include "harness.h"

/*
 * This file does not include include/libvcc/builtins.h on purpose. It is the
 * caller's side of the 128-bit ABI: the declarations below spell the type the
 * compiler emits a call with, which is unsigned __int128, and the definitions
 * they reach are the two-word pair in the library. If the pair and the type
 * were not the same object on the wire, these calls would answer numbers a
 * shift does not: the argument registers would be read in the wrong order, or
 * the pointer would be read from the third word instead of the third register.
 *
 * The expected values are the compiler's own arithmetic, which is not the
 * library's: shifts and a multiply are instructions here, and the division
 * cases use only powers of two, whose quotient is a shift and whose remainder
 * is a mask.
 */

#define CHECK_U128(got, want) \
	do { \
		CHECK_U64((unsigned long long)(got), (unsigned long long)(want)); \
		CHECK_U64((unsigned long long)((got) >> 64), \
		          (unsigned long long)((want) >> 64)); \
	} while (0)

#if LIBVCC_HAVE_INT128

typedef unsigned __int128 u128;
typedef __int128 i128;

extern u128 __ashlti3(u128 a, int b);
extern u128 __ashrti3(u128 a, int b);
extern u128 __lshrti3(u128 a, int b);
extern u128 __multi3(u128 a, u128 b);
extern int __clzti2(u128 x);
extern int __ctzti2(u128 x);
extern int __popcountti2(u128 x);
extern int __parityti2(u128 x);
extern u128 __udivmodti4(u128 n, u128 d, u128 *rem);
extern u128 __udivti3(u128 n, u128 d);
extern u128 __umodti3(u128 n, u128 d);
extern u128 __divmodti4(u128 a, u128 b, u128 *rem);
extern u128 __divti3(u128 a, u128 b);
extern u128 __modti3(u128 a, u128 b);

/* A value the compiler cannot fold: the calls have to be in the binary for
   this file to be testing anything. */
static volatile unsigned long long seed = 0x243f6a8885a308d3ULL;

static unsigned long long next(void)
{
	unsigned long long x;

	x = seed;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	seed = x;
	return x;
}

static u128 make(void)
{
	return ((u128)next() << 64) | next();
}

static int ref_popcount(u128 v)
{
	unsigned long long lo, hi;
	int n;

	lo = (unsigned long long)v;
	hi = (unsigned long long)(v >> 64);
	n = 0;
	while (lo != 0) {
		lo &= lo - 1;
		n++;
	}
	while (hi != 0) {
		hi &= hi - 1;
		n++;
	}
	return n;
}

static int ref_clz(u128 v)
{
	unsigned long long w;
	int n;

	n = 0;
	w = (unsigned long long)(v >> 64);
	if (w == 0) {
		n = 64;
		w = (unsigned long long)v;
	}
	if (w == 0)
		return 128;
	while ((w & 0x8000000000000000ULL) == 0) {
		w <<= 1;
		n++;
	}
	return n;
}

static int ref_ctz(u128 v)
{
	unsigned long long w;
	int n;

	n = 0;
	w = (unsigned long long)v;
	if (w == 0) {
		n = 64;
		w = (unsigned long long)(v >> 64);
	}
	if (w == 0)
		return 128;
	while ((w & 1ULL) == 0) {
		w >>= 1;
		n++;
	}
	return n;
}

void libvcc_test_abi128(void)
{
	u128 a, b, d, q, r, positive;
	i128 sa;
	int i, k;

	for (i = 0; i < 64; i++) {
		a = make();
		b = make();

		for (k = 0; k < 128; k += 5) {
			CHECK_U128(__ashlti3(a, k), a << k);
			CHECK_U128(__lshrti3(a, k), a >> k);
			sa = (i128)a;
			CHECK_U128(__ashrti3(a, k), (u128)(sa >> k));
		}

		/* A multiply is one instruction on this target, not __multi3. */
		CHECK_U128(__multi3(a, b), a * b);

		/* Counts, which need no 128-bit arithmetic to check. */
		CHECK_U64((unsigned long long)__clzti2(a),
		          (unsigned long long)ref_clz(a));
		CHECK_U64((unsigned long long)__ctzti2(a),
		          (unsigned long long)ref_ctz(a));
		CHECK_U64((unsigned long long)__popcountti2(a),
		          (unsigned long long)ref_popcount(a));
		CHECK(__parityti2(a) == (ref_popcount(a) & 1));

		/* Division by 2^k: a shift and a mask, not a division. */
		for (k = 0; k < 128; k += 7) {
			d = (u128)1 << k;
			q = __udivmodti4(a, d, &r);
			CHECK_U128(q, a >> k);
			CHECK_U128(r, a & (d - 1));
			CHECK_U128(__udivti3(a, d), a >> k);
			CHECK_U128(__umodti3(a, d), a & (d - 1));

			positive = a & ((((u128)1) << 127) - 1);
			q = __divmodti4(positive, d, &r);
			CHECK_U128(q, positive >> k);
			CHECK_U128(r, positive & (d - 1));
			CHECK_U128(__divti3(positive, d), positive >> k);
			CHECK_U128(__modti3(positive, d), positive & (d - 1));
		}
	}
}

#else /* vcc */

void libvcc_test_abi128(void)
{
	/*
	 * Nothing to spell. vcc parses the type and has no values of its width,
	 * so a call emitted by vcc is not a thing that exists yet; the pair the
	 * library defines is still built and still tested by the groups beside
	 * this one, through the ABI header.
	 */
}

#endif /* LIBVCC_HAVE_INT128 */
