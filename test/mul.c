#include <libvcc/builtins.h>

#include "harness.h"

/* The low 64 bits are what the ABI asks for, and that is what the machine's
   own multiply leaves behind too. */

static void check(long long a, long long b)
{
	CHECK_U64(__muldi3(a, b),
	          (unsigned long long)a * (unsigned long long)b);
}

void libvcc_test_mul(void)
{
	static const long long edges[] = {
		0LL, 1LL, -1LL, 2LL, -2LL, 3LL, -3LL, 0x10000LL, -0x10000LL,
		0xffffffffLL, 0x100000000LL, 0x7fffffffLL, -0x80000000LL,
		LIBVCC_LL_MAX, LIBVCC_LL_MIN, LIBVCC_LL_MIN + 1
	};
	unsigned long long state;
	long long a, b;
	unsigned long i, j;

	for (i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
		for (j = 0; j < sizeof(edges) / sizeof(edges[0]); j++)
			check(edges[i], edges[j]);

	state = 0xa4093822299f31d0ULL;
	for (i = 0; i < 4000; i++) {
		a = (long long)libvcc_test_random(&state);
		b = (long long)libvcc_test_random(&state);
		check(a, b);
	}
}
