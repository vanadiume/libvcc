#include <libvcc/builtins.h>

#include "harness.h"

void libvcc_test_shift(void)
{
	unsigned long long state, a;
	int b, i;

	state = 0x243f6a8885a308d3ULL;
	for (b = 0; b < 64; b++) {
		for (i = 0; i < 64; i++) {
			a = libvcc_test_random(&state);
			CHECK_U64(__ashldi3((long long)a, b), a << b);
			CHECK_U64(__lshrdi3((long long)a, b), a >> b);
			CHECK_LL(__ashrdi3((long long)a, b), (long long)a >> b);
		}
	}

	CHECK_U64(__ashldi3(1LL, 0), 1ULL);
	CHECK_U64(__ashldi3(1LL, 31), 0x80000000ULL);
	CHECK_U64(__ashldi3(1LL, 32), 0x100000000ULL);
	CHECK_U64(__ashldi3(1LL, 63), 0x8000000000000000ULL);
	CHECK_U64(__lshrdi3((long long)0xffffffffffffffffULL, 1),
	          0x7fffffffffffffffULL);
	CHECK_U64(__lshrdi3((long long)0xffffffffffffffffULL, 32),
	          0xffffffffULL);
	CHECK_U64(__lshrdi3((long long)0xffffffffffffffffULL, 63), 1ULL);
	CHECK_LL(__ashrdi3(-1LL, 1), -1LL);
	CHECK_LL(__ashrdi3(-2LL, 1), -1LL);
	CHECK_LL(__ashrdi3(-3LL, 1), -2LL);
	CHECK_LL(__ashrdi3(5LL, 1), 2LL);
	CHECK_LL(__ashrdi3(LIBVCC_LL_MIN, 32), LIBVCC_LL_MIN >> 32);
	CHECK_LL(__ashrdi3(LIBVCC_LL_MIN, 63), -1LL);
	CHECK_LL(__ashrdi3(LIBVCC_LL_MAX, 62), 1LL);
}
