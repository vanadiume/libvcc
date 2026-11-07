#include <libvcc/builtins.h>

#include "harness.h"
#include "wide.h"

/*
 * A count is checked against a loop over the raw bits, which is not how the
 * helper works: the helper hands each word to the 64-bit helper beside it.
 */

static int ref_clz(libvcc_u128 v)
{
	int n;
	unsigned long long w;

	if (v.lo == 0 && v.hi == 0)
		return 128;
	n = 0;
	w = v.hi;
	if (w == 0) {
		n = 64;
		w = v.lo;
	}
	while ((w & 0x8000000000000000ULL) == 0) {
		w <<= 1;
		n++;
	}
	return n;
}

static int ref_ctz(libvcc_u128 v)
{
	int n;
	unsigned long long w;

	if (v.lo == 0 && v.hi == 0)
		return 128;
	n = 0;
	w = v.lo;
	if (w == 0) {
		n = 64;
		w = v.hi;
	}
	while ((w & 1ULL) == 0) {
		w >>= 1;
		n++;
	}
	return n;
}

static int ref_popcount(libvcc_u128 v)
{
	int n;
	unsigned long long w;

	n = 0;
	w = v.lo;
	while (w != 0) {
		w &= w - 1ULL;
		n++;
	}
	w = v.hi;
	while (w != 0) {
		w &= w - 1ULL;
		n++;
	}
	return n;
}

static int ref_parity(libvcc_u128 v)
{
	int p;
	unsigned long long w;

	p = 0;
	w = v.lo;
	while (w != 0) {
		p ^= 1;
		w &= w - 1ULL;
	}
	w = v.hi;
	while (w != 0) {
		p ^= 1;
		w &= w - 1ULL;
	}
	return p;
}

static void check(libvcc_u128 v)
{
	CHECK(__clzti2(v) == ref_clz(v));
	CHECK(__ctzti2(v) == ref_ctz(v));
	CHECK(__popcountti2(v) == ref_popcount(v));
	CHECK(__parityti2(v) == ref_parity(v));
}

void libvcc_test_bits128(void)
{
	static const struct {
		unsigned long long lo, hi;
	} edges[] = {
		{ 0ULL, 0ULL },
		{ 1ULL, 0ULL },
		{ 2ULL, 0ULL },
		{ 3ULL, 0ULL },
		{ 0x8000000000000000ULL, 0ULL },
		{ 0xffffffffffffffffULL, 0ULL },
		{ 0ULL, 1ULL },
		{ 1ULL, 1ULL },
		{ 0ULL, 0x8000000000000000ULL },
		{ 0ULL, 0xffffffffffffffffULL },
		{ 0xffffffffffffffffULL, 0xffffffffffffffffULL },
		{ 0xffffffffffffffffULL, 0x7fffffffffffffffULL },
		{ 0x8000000000000001ULL, 0x8000000000000000ULL }
	};
	unsigned long long state;
	unsigned long i;

	for (i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
		check(WIDE(edges[i].lo, edges[i].hi));

	/* Every single bit, on both sides of the word boundary. */
	for (i = 0; i < 128; i++) {
		if (i < 64)
			check(WIDE(1ULL << i, 0ULL));
		else
			check(WIDE(0ULL, 1ULL << (i - 64)));
	}

	state = 0xa4093822299f31d0ULL;
	for (i = 0; i < 2000; i++)
		check(libvcc_test_wide_random(&state));

	/* And values whose high word is zero, for the branch that adds 64. */
	for (i = 0; i < 500; i++)
		check(WIDE(libvcc_test_random(&state), 0ULL));
}
