#include <stdio.h>

#include "harness.h"
#include "wide.h"

libvcc_u128 libvcc_test_wide(unsigned long long lo, unsigned long long hi)
{
	libvcc_u128 v;

	v.lo = lo;
	v.hi = hi;
	return v;
}

libvcc_u128 libvcc_test_wide_random(unsigned long long *state)
{
	libvcc_u128 v;

	v.lo = libvcc_test_random(state);
	v.hi = libvcc_test_random(state);
	return v;
}

void libvcc_test_check_wide(const char *file, int line, const char *what,
                            libvcc_u128 got, libvcc_u128 want)
{
	libvcc_test_checks++;
	if (got.lo == want.lo && got.hi == want.hi)
		return;
	libvcc_test_failures++;
	fputs("FAIL ", stdout);
	fputs(file, stdout);
	fputc(':', stdout);
	libvcc_test_put_u64((unsigned long long)line);
	fputs(": ", stdout);
	fputs(what, stdout);
	fputs(": got hi:", stdout);
	libvcc_test_put_u64(got.hi);
	fputs(" lo:", stdout);
	libvcc_test_put_u64(got.lo);
	fputs(" want hi:", stdout);
	libvcc_test_put_u64(want.hi);
	fputs(" lo:", stdout);
	libvcc_test_put_u64(want.lo);
	fputc('\n', stdout);
}

/*
 * One 64-bit by 64-bit product, in 32-bit pieces. The test binary is built by
 * the same compilers as the library, and a 64-bit multiply in the test is an
 * instruction on the machine the matrix runs on; the pieces are here so the
 * reference is arithmetic that no helper could have supplied.
 */

static void mul64_ref(unsigned long long a, unsigned long long b,
                      unsigned long long *hi, unsigned long long *lo)
{
	unsigned long long a0, a1, b0, b1, p00, p01, p10, p11, mid;

	a0 = a & 0xffffffffULL;
	a1 = a >> 32;
	b0 = b & 0xffffffffULL;
	b1 = b >> 32;
	p00 = a0 * b0;
	p01 = a0 * b1;
	p10 = a1 * b0;
	p11 = a1 * b1;
	mid = (p00 >> 32) + (p01 & 0xffffffffULL) + (p10 & 0xffffffffULL);
	*lo = (p00 & 0xffffffffULL) | (mid << 32);
	*hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
}

void libvcc_test_mul_wide(libvcc_u128 a, libvcc_u128 b, libvcc_u128 *out,
                          int *carry)
{
	unsigned long long h0, l0, h1, l1, h2, l2, hi;

	mul64_ref(a.lo, b.lo, &h0, &l0);
	mul64_ref(a.lo, b.hi, &h1, &l1);
	mul64_ref(a.hi, b.lo, &h2, &l2);
	*carry = 0;
	out->lo = l0;
	hi = h0 + l1;
	if (hi < h0)
		*carry = 1;
	h0 = hi;
	hi = h0 + l2;
	if (hi < h0)
		*carry = 1;
	out->hi = hi;
	if (h1 != 0 || h2 != 0)
		*carry = 1;
}
