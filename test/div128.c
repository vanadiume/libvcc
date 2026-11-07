#include <libvcc/builtins.h>

#include "harness.h"
#include "wide.h"

/*
 * There is no machine instruction to compare a 128-bit division against, so
 * the comparison is the definition: for a divisor of zero or more, at most one
 * pair (q, r) satisfies n == q * d + r and r < d. Checking that pair proves
 * the answer rather than agreeing with it, and the product is computed by a
 * reference the library does not use.
 *
 * Divisors of the form 2^k are also checked against a shift, which is an
 * operation the division is not written with.
 */

static libvcc_u128 shr1(libvcc_u128 v)
{
	libvcc_u128 r;
	unsigned long long carry;

	carry = v.hi << 63;
	r.lo = (v.lo >> 1) | carry;
	r.hi = v.hi >> 1;
	return r;
}

static libvcc_u128 shr_value(libvcc_u128 v, int b)
{
	while (b > 0) {
		v = shr1(v);
		b--;
	}
	return v;
}

static libvcc_u128 add_ref(libvcc_u128 a, libvcc_u128 b, int *carry)
{
	libvcc_u128 r;
	int c;

	r.lo = a.lo + b.lo;
	c = r.lo < a.lo ? 1 : 0;
	r.hi = a.hi + b.hi;
	*carry = r.hi < a.hi ? 1 : 0;
	r.hi = r.hi + (unsigned long long)c;
	if (r.hi < (unsigned long long)c)
		*carry = 1;
	return r;
}

static int ref_less(libvcc_u128 a, libvcc_u128 b)
{
	if (a.hi != b.hi)
		return a.hi < b.hi;
	return a.lo < b.lo;
}

static int is_zero(libvcc_u128 v)
{
	return v.lo == 0 && v.hi == 0;
}

/* The quotient and remainder, proved by the identity and the bound. */
static void check_unsigned(libvcc_u128 n, libvcc_u128 d)
{
	libvcc_u128 q, r, prod, sum;
	int carry;

	if (is_zero(d))
		return;
	q = __udivmodti4(n, d, &r);
	CHECK_WIDE(__udivti3(n, d), q);
	CHECK_WIDE(__umodti3(n, d), r);
	CHECK(ref_less(r, d));
	libvcc_test_mul_wide(q, d, &prod, &carry);
	CHECK(carry == 0);
	sum = add_ref(prod, r, &carry);
	CHECK(carry == 0);
	CHECK_WIDE(sum, n);
}

static int negative(libvcc_u128 v)
{
	return (v.hi & 0x8000000000000000ULL) != 0;
}

static libvcc_u128 neg_ref(libvcc_u128 v)
{
	libvcc_u128 r;

	r.lo = 0ULL - v.lo;
	r.hi = ~v.hi + (v.lo == 0 ? 1ULL : 0ULL);
	return r;
}

static void check_signed(libvcc_u128 a, libvcc_u128 b)
{
	libvcc_u128 q, r, prod, sum;
	libvcc_u128 abs_a, abs_b, abs_q, abs_r;
	int carry;

	if (is_zero(b))
		return;
	/* The most negative value over -1 has no answer of that width. */
	if (a.lo == 0 && a.hi == 0x8000000000000000ULL &&
	    b.lo == 0xffffffffffffffffULL && b.hi == 0xffffffffffffffffULL)
		return;
	q = __divmodti4(a, b, &r);
	CHECK_WIDE(__divti3(a, b), q);
	CHECK_WIDE(__modti3(a, b), r);

	/*
	 * The identity is checked on magnitudes. The product of the two bit
	 * patterns overflows the pair the moment either is negative, so a
	 * carry out of that multiply says nothing about the quotient; the
	 * magnitudes are unsigned, and there the carry is the answer.
	 */
	abs_a = negative(a) ? neg_ref(a) : a;
	abs_b = negative(b) ? neg_ref(b) : b;
	abs_q = negative(q) ? neg_ref(q) : q;
	abs_r = negative(r) ? neg_ref(r) : r;
	libvcc_test_mul_wide(abs_q, abs_b, &prod, &carry);
	CHECK(carry == 0);
	sum = add_ref(prod, abs_r, &carry);
	CHECK(carry == 0);
	CHECK_WIDE(sum, abs_a);

	/* |r| < |b|, and each result carries the sign that fixes it. */
	CHECK(ref_less(abs_r, abs_b));
	if (!is_zero(q))
		CHECK(negative(q) == (negative(a) != negative(b)));
	if (!is_zero(r))
		CHECK(negative(r) == negative(a));
}

void libvcc_test_div128(void)
{
	static const struct {
		unsigned long long lo, hi;
	} edges[] = {
		{ 0ULL, 0ULL },
		{ 1ULL, 0ULL },
		{ 2ULL, 0ULL },
		{ 3ULL, 0ULL },
		{ 7ULL, 0ULL },
		{ 0xffffffffffffffffULL, 0ULL },
		{ 0ULL, 1ULL },
		{ 1ULL, 1ULL },
		{ 0xffffffffffffffffULL, 0xffffffffffffffffULL },
		{ 0ULL, 0x8000000000000000ULL },
		{ 1ULL, 0x8000000000000000ULL },
		{ 0xffffffffffffffffULL, 0x7fffffffffffffffULL },
		{ 0x8000000000000000ULL, 0x8000000000000000ULL },
		{ 0xffffffffffffffffULL, 0x00000000ffffffffULL }
	};
	static const struct {
		unsigned long long lo, hi;
	} signed_edges[] = {
		{ 0ULL, 0ULL },
		{ 1ULL, 0ULL },
		{ 0xffffffffffffffffULL, 0xffffffffffffffffULL },
		{ 2ULL, 0ULL },
		{ 0xfffffffffffffffeULL, 0xffffffffffffffffULL },
		{ 7ULL, 0ULL },
		{ 0xfffffffffffffff9ULL, 0xffffffffffffffffULL },
		{ 100ULL, 0ULL },
		{ 0xffffffffffffff9cULL, 0xffffffffffffffffULL },
		{ 0ULL, 1ULL },
		{ 0ULL, 0xffffffffffffffffULL },
		{ 0ULL, 0x8000000000000000ULL },
		{ 1ULL, 0x8000000000000000ULL },
		{ 0xffffffffffffffffULL, 0x7fffffffffffffffULL },
		{ 0xffffffffffffffffULL, 0ULL },
		{ 0x123456789abcdef0ULL, 0x0fedcba987654321ULL }
	};
	unsigned long long state;
	libvcc_u128 n, d, q, r;
	unsigned long i, j;
	int k;

	for (i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
		for (j = 0; j < sizeof(edges) / sizeof(edges[0]); j++)
			check_unsigned(WIDE(edges[i].lo, edges[i].hi),
			               WIDE(edges[j].lo, edges[j].hi));

	state = 0x13198a2e03707344ULL;
	for (i = 0; i < 1500; i++) {
		n = libvcc_test_wide_random(&state);
		d = libvcc_test_wide_random(&state);
		check_unsigned(n, d);
	}

	/* Every divisor of the form 2^k: the quotient is a shift. */
	for (k = 0; k < 128; k++) {
		if (k < 64)
			d = WIDE(1ULL << k, 0ULL);
		else
			d = WIDE(0ULL, 1ULL << (k - 64));
		n = WIDE(0xffffffffffffffffULL, 0xffffffffffffffffULL);
		q = __udivmodti4(n, d, &r);
		CHECK_WIDE(q, shr_value(n, k));
		check_unsigned(libvcc_test_wide_random(&state), d);
		check_unsigned(n, d);
	}

	/* Small answers a person can read. */
	q = __udivmodti4(WIDE(300ULL, 0ULL), WIDE(7ULL, 0ULL), &r);
	CHECK_WIDE(q, WIDE(42ULL, 0ULL));
	CHECK_WIDE(r, WIDE(6ULL, 0ULL));

	for (i = 0; i < sizeof(signed_edges) / sizeof(signed_edges[0]); i++)
		for (j = 0; j < sizeof(signed_edges) / sizeof(signed_edges[0]); j++)
			check_signed(WIDE(signed_edges[i].lo, signed_edges[i].hi),
			             WIDE(signed_edges[j].lo, signed_edges[j].hi));

	for (i = 0; i < 1500; i++) {
		n = libvcc_test_wide_random(&state);
		d = libvcc_test_wide_random(&state);
		check_signed(n, d);
	}

	/* A zero divisor answers zero and the dividend, as the ABI says. */
	CHECK_WIDE(__udivmodti4(WIDE(7ULL, 1ULL), WIDE(0ULL, 0ULL), &r),
	           WIDE(0ULL, 0ULL));
	CHECK_WIDE(r, WIDE(7ULL, 1ULL));
	CHECK_WIDE(__divmodti4(WIDE(7ULL, 1ULL), WIDE(0ULL, 0ULL), &r),
	           WIDE(0ULL, 0ULL));
	CHECK_WIDE(r, WIDE(7ULL, 1ULL));
}
