#include <libvcc/builtins.h>

#include "harness.h"

static void check_unsigned(unsigned int n, unsigned int d)
{
	unsigned int q, r;

	if (d == 0)
		return;
	q = __udivmodsi4(n, d, &r);
	CHECK_U64(q, n / d);
	CHECK_U64(r, n % d);
	CHECK_U64(__udivsi3(n, d), n / d);
	CHECK_U64(__umodsi3(n, d), n % d);
}

static void check_signed(int a, int b)
{
	if (b == 0)
		return;
	if (a == LIBVCC_INT_MIN && b == -1)
		return;
	CHECK_LL(__divsi3(a, b), a / b);
	CHECK_LL(__modsi3(a, b), a % b);
}

void libvcc_test_div32(void)
{
	static const unsigned int edges[] = {
		0U, 1U, 2U, 3U, 7U, 10U, 255U, 256U, 65535U, 65536U,
		0x7fffffffU, 0x80000000U, 0x80000001U, 0xfffffffeU, 0xffffffffU
	};
	static const int signed_edges[] = {
		0, 1, -1, 2, -2, 7, -7, 100, -100,
		LIBVCC_INT_MAX, LIBVCC_INT_MIN, LIBVCC_INT_MIN + 1, -32768, 32768
	};
	unsigned long long state;
	unsigned int n, d, k;
	int a, b;
	unsigned long i, j;

	for (i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
		for (j = 0; j < sizeof(edges) / sizeof(edges[0]); j++)
			check_unsigned(edges[i], edges[j]);

	state = 0x2545f4914f6cdd1dULL;
	for (i = 0; i < 4000; i++) {
		n = (unsigned int)libvcc_test_random(&state);
		d = (unsigned int)libvcc_test_random(&state);
		check_unsigned(n, d);
	}

	for (k = 0; k < 32; k++) {
		d = (unsigned int)1 << k;
		check_unsigned(0xffffffffU, d);
		check_unsigned(0x80000000U, d);
		check_unsigned(d, d);
	}

	for (i = 0; i < sizeof(signed_edges) / sizeof(signed_edges[0]); i++)
		for (j = 0; j < sizeof(signed_edges) / sizeof(signed_edges[0]); j++)
			check_signed(signed_edges[i], signed_edges[j]);

	for (i = 0; i < 4000; i++) {
		a = (int)libvcc_test_random(&state);
		b = (int)libvcc_test_random(&state);
		check_signed(a, b);
	}
}
