#include <libvcc/builtins.h>

#include "harness.h"

/*
 * The expected values come from the machine's own division. On x86-64 that is
 * one instruction and not a call back into the helper, so the comparison is
 * against the hardware's answer and not against a second copy of this code.
 */

static void check_unsigned(unsigned long long n, unsigned long long d)
{
	unsigned long long q, r;

	if (d == 0)
		return;
	q = __udivmoddi4(n, d, &r);
	CHECK_U64(q, n / d);
	CHECK_U64(r, n % d);
	CHECK_U64(__udivdi3(n, d), n / d);
	CHECK_U64(__umoddi3(n, d), n % d);
}

static void check_signed(long long a, long long b)
{
	if (b == 0)
		return;
	if (a == LIBVCC_LL_MIN && b == -1)
		return; /* the quotient is not representable */
	CHECK_LL(__divdi3(a, b), a / b);
	CHECK_LL(__moddi3(a, b), a % b);
}

void libvcc_test_div64(void)
{
	static const unsigned long long edges[] = {
		0ULL, 1ULL, 2ULL, 3ULL, 7ULL, 10ULL, 255ULL, 256ULL,
		65535ULL, 65536ULL, 0x7fffffffffffffffULL, 0x8000000000000000ULL,
		0x8000000000000001ULL, 0xfffffffffffffffeULL,
		0xffffffffffffffffULL
	};
	static const long long signed_edges[] = {
		0LL, 1LL, -1LL, 2LL, -2LL, 7LL, -7LL, 100LL, -100LL,
		LIBVCC_LL_MAX, LIBVCC_LL_MIN, LIBVCC_LL_MIN + 1, -32768LL, 32768LL
	};
	unsigned long long state, n, d;
	long long a, b;
	unsigned long i, j;

	for (i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
		for (j = 0; j < sizeof(edges) / sizeof(edges[0]); j++)
			check_unsigned(edges[i], edges[j]);

	state = 0x9e3779b97f4a7c15ULL;
	for (i = 0; i < 4000; i++) {
		n = libvcc_test_random(&state);
		d = libvcc_test_random(&state);
		check_unsigned(n, d);
	}

	/* Every divisor of the form 2^k, which takes the alignment branch. */
	for (j = 0; j < 64; j++) {
		d = (unsigned long long)1 << j;
		check_unsigned(0xffffffffffffffffULL, d);
		check_unsigned(0x8000000000000000ULL, d);
		check_unsigned(d, d);
	}

	for (i = 0; i < sizeof(signed_edges) / sizeof(signed_edges[0]); i++)
		for (j = 0; j < sizeof(signed_edges) / sizeof(signed_edges[0]); j++)
			check_signed(signed_edges[i], signed_edges[j]);

	for (i = 0; i < 4000; i++) {
		a = (long long)libvcc_test_random(&state);
		b = (long long)libvcc_test_random(&state);
		check_signed(a, b);
	}

	/* A zero divisor is the one case the ABI does not define. */
	CHECK_U64(__udivmoddi4(7ULL, 0ULL, &d), 0ULL);
	CHECK_U64(d, 7ULL);
}
