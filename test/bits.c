#include <libvcc/builtins.h>

#include "harness.h"

/*
 * The references are written a different way from the helpers on purpose: a
 * loop that shifts x right and counts is not the loop that shifts x left, so
 * a mistake in one does not agree with the same mistake in the other.
 */

static int ref_clz64(unsigned long long x)
{
	int n;

	n = 0;
	while (x != 0) {
		n++;
		x >>= 1;
	}
	return 64 - n;
}

static int ref_clz32(unsigned int x)
{
	int n;

	n = 0;
	while (x != 0) {
		n++;
		x >>= 1;
	}
	return 32 - n;
}

static int ref_ctz64(unsigned long long x)
{
	int n;

	n = 0;
	while (n < 64) {
		if (((x >> n) & 1ULL) != 0)
			break;
		n++;
	}
	return n;
}

static int ref_ctz32(unsigned int x)
{
	int n;

	n = 0;
	while (n < 32) {
		if (((x >> n) & 1U) != 0)
			break;
		n++;
	}
	return n;
}

static int ref_popcount64(unsigned long long x)
{
	int i, n;

	n = 0;
	for (i = 0; i < 64; i++) {
		if (((x >> i) & 1ULL) != 0)
			n++;
	}
	return n;
}

static int ref_popcount32(unsigned int x)
{
	int i, n;

	n = 0;
	for (i = 0; i < 32; i++) {
		if (((x >> i) & 1U) != 0)
			n++;
	}
	return n;
}

static unsigned long long ref_bswap64(unsigned long long x)
{
	unsigned long long r;
	int i;

	r = 0;
	for (i = 0; i < 8; i++) {
		r <<= 8;
		r |= x & 0xffULL;
		x >>= 8;
	}
	return r;
}

static unsigned int ref_bswap32(unsigned int x)
{
	unsigned int r;
	int i;

	r = 0;
	for (i = 0; i < 4; i++) {
		r <<= 8;
		r |= x & 0xffU;
		x >>= 8;
	}
	return r;
}

void libvcc_test_bits(void)
{
	static const unsigned long long edges[] = {
		0ULL, 1ULL, 2ULL, 3ULL, 0xffULL, 0x100ULL, 0x80000000ULL,
		0xffffffffULL, 0x100000000ULL, 0x0123456789abcdefULL,
		0x8000000000000000ULL, 0xffffffffffffffffULL,
		0xfffffffffffffffeULL, 0x00000000ffffffffULL
	};
	unsigned long long state, x64, y64;
	unsigned int x32;
	unsigned long i;

	for (i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
		x64 = edges[i];
		x32 = (unsigned int)edges[i];
		CHECK_U64(__clzdi2(x64), ref_clz64(x64));
		CHECK_U64(__ctzdi2(x64), ref_ctz64(x64));
		CHECK_U64(__popcountdi2(x64), ref_popcount64(x64));
		CHECK_U64(__paritydi2(x64), ref_popcount64(x64) & 1);
		y64 = ref_bswap64(x64);
		CHECK_U64(__bswapdi2(x64), y64);
		CHECK_U64(__bswapdi2(y64), x64);

		CHECK_U64(__clzsi2(x32), ref_clz32(x32));
		CHECK_U64(__ctzsi2(x32), ref_ctz32(x32));
		CHECK_U64(__popcountsi2(x32), ref_popcount32(x32));
		CHECK_U64(__paritysi2(x32), ref_popcount32(x32) & 1);
		CHECK_U64(__bswapsi2(x32), ref_bswap32(x32));
		CHECK_U64(__bswapsi2(ref_bswap32(x32)), x32);
	}

	state = 0x13198a2e03707344ULL;
	for (i = 0; i < 2000; i++) {
		x64 = libvcc_test_random(&state);
		x32 = (unsigned int)libvcc_test_random(&state);
		CHECK_U64(__clzdi2(x64), ref_clz64(x64));
		CHECK_U64(__ctzdi2(x64), ref_ctz64(x64));
		CHECK_U64(__popcountdi2(x64), ref_popcount64(x64));
		CHECK_U64(__paritydi2(x64), ref_popcount64(x64) & 1);
		CHECK_U64(__bswapdi2(x64), ref_bswap64(x64));

		CHECK_U64(__clzsi2(x32), ref_clz32(x32));
		CHECK_U64(__ctzsi2(x32), ref_ctz32(x32));
		CHECK_U64(__popcountsi2(x32), ref_popcount32(x32));
		CHECK_U64(__paritysi2(x32), ref_popcount32(x32) & 1);
		CHECK_U64(__bswapsi2(x32), ref_bswap32(x32));
	}

	/* The two known byte orders, so a reference and a helper that share a
	   mistake still have something to answer to. */
	CHECK_U64(__bswapdi2(0x0123456789abcdefULL), 0xefcdab8967452301ULL);
	CHECK_U64(__bswapsi2(0x01234567U), 0x67452301U);
}
