#include <libvcc/builtins.h>

#include "harness.h"
#include "wide.h"

/*
 * The reference is test/wide.c's: three 64-bit multiplies where the library
 * does a 16-bit convolution. Different arithmetic, same answer required.
 */

static void check(libvcc_u128 a, libvcc_u128 b)
{
	libvcc_u128 want;
	int carry;

	libvcc_test_mul_wide(a, b, &want, &carry);
	CHECK_WIDE(__multi3(a, b), want);
}

void libvcc_test_mul128(void)
{
	unsigned long long state;
	libvcc_u128 a, b, want;
	unsigned long i;
	int carry;

	CHECK_WIDE(__multi3(WIDE(0ULL, 0ULL), WIDE(0ULL, 0ULL)),
	           WIDE(0ULL, 0ULL));
	CHECK_WIDE(__multi3(WIDE(1ULL, 0ULL), WIDE(1ULL, 0ULL)),
	           WIDE(1ULL, 0ULL));
	CHECK_WIDE(__multi3(WIDE(0xffffffffffffffffULL, 0ULL),
	                    WIDE(0xffffffffffffffffULL, 0ULL)),
	           WIDE(1ULL, 0xfffffffffffffffeULL));

	/* The limbs in the middle, where a convolution drops or duplicates one. */
	CHECK_WIDE(__multi3(WIDE(0x00000000ffffffffULL, 0ULL),
	                    WIDE(0x00000000ffffffffULL, 0ULL)),
	           WIDE(0xfffffffe00000001ULL, 0ULL));
	CHECK_WIDE(__multi3(WIDE(0ULL, 0x00000000ffffffffULL),
	                    WIDE(0ULL, 0x00000000ffffffffULL)),
	           WIDE(0ULL, 0ULL));
	CHECK_WIDE(__multi3(WIDE(0ULL, 0x0000000000000001ULL),
	                    WIDE(0ULL, 0x0000000000000001ULL)),
	           WIDE(0ULL, 0ULL));
	CHECK_WIDE(__multi3(WIDE(0ULL, 0x8000000000000000ULL), WIDE(2ULL, 0ULL)),
	           WIDE(0ULL, 0ULL));

	/* A product that fills the low word and carries into the high one. */
	CHECK_WIDE(__multi3(WIDE(0xffffffffffffffffULL, 0ULL),
	                    WIDE(0xffffffffffffffffULL, 0ULL)),
	           WIDE(1ULL, 0xfffffffffffffffeULL));

	state = 0x082efa98ec4e6c89ULL;
	for (i = 0; i < 4000; i++) {
		a = WIDE(libvcc_test_random(&state), libvcc_test_random(&state));
		b = WIDE(libvcc_test_random(&state), libvcc_test_random(&state));
		check(a, b);
	}

	/* Values whose high half is zero, which is where a carry is not consumed. */
	for (i = 0; i < 1000; i++) {
		a = WIDE(libvcc_test_random(&state), 0ULL);
		b = WIDE(libvcc_test_random(&state), 0ULL);
		check(a, b);
	}

	/* And every limb set, including the top pair. The product is wider than
	   the answer, which is the case the helper truncates. */
	a = WIDE(0xffffffffffffffffULL, 0xffffffffffffffffULL);
	b = WIDE(0x0101010101010101ULL, 0x0101010101010101ULL);
	libvcc_test_mul_wide(a, b, &want, &carry);
	CHECK(carry == 1);
	CHECK_WIDE(__multi3(a, b), want);
}
