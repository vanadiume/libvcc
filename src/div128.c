#include <libvcc/builtins.h>

/*
 * Restoring division over the two words, one quotient bit at a time, with the
 * divisor doubled until its top set bit lines up with the dividend's. The
 * double is what keeps this from overflowing: the shifted divisor is at most
 * the dividend, so the top bit never leaves the pair.
 *
 * Nothing here divides. That is the whole point of the file.
 *
 * The alignment walks the divisor up one bit at a time rather than shifting it
 * once by the difference of the two counts. A shift by a count in 0..127 is
 * the helper beside this one, and calling it would make the division depend on
 * a second file for the one step it cannot get wrong by itself.
 */

static int wide_is_zero(libvcc_u128 v)
{
	return v.lo == 0 && v.hi == 0;
}

static int wide_less(libvcc_u128 a, libvcc_u128 b)
{
	if (a.hi != b.hi)
		return a.hi < b.hi;
	return a.lo < b.lo;
}

static libvcc_u128 wide_shl1(libvcc_u128 v)
{
	libvcc_u128 r;
	unsigned long long carry;

	carry = v.lo >> 63;
	r.lo = v.lo << 1;
	r.hi = (v.hi << 1) | carry;
	return r;
}

static libvcc_u128 wide_shr1(libvcc_u128 v)
{
	libvcc_u128 r;
	unsigned long long carry;

	carry = v.hi << 63;
	r.lo = (v.lo >> 1) | carry;
	r.hi = v.hi >> 1;
	return r;
}

static libvcc_u128 wide_sub(libvcc_u128 a, libvcc_u128 b)
{
	libvcc_u128 r;
	unsigned long long borrow;

	borrow = a.lo < b.lo ? 1ULL : 0ULL;
	r.lo = a.lo - b.lo;
	r.hi = a.hi - b.hi - borrow;
	return r;
}

/* The width for zero, like __clzdi2: 128, not undefined. */
static int wide_clz(libvcc_u128 v)
{
	if (v.hi != 0)
		return __clzdi2(v.hi);
	return 64 + __clzdi2(v.lo);
}

libvcc_u128 __udivmodti4(libvcc_u128 n, libvcc_u128 d, libvcc_u128 *rem)
{
	libvcc_u128 q, ds;
	unsigned long long zero;
	int shift, i;

	zero = 0;
	q.lo = zero;
	q.hi = zero;
	if (wide_is_zero(d)) {
		if (rem != 0)
			*rem = n;
		return q;
	}
	if (wide_less(n, d)) {
		if (rem != 0)
			*rem = n;
		return q;
	}
	shift = wide_clz(d) - wide_clz(n);
	ds = d;
	for (i = 0; i < shift; i++)
		ds = wide_shl1(ds);
	for (;;) {
		q = wide_shl1(q);
		if (!wide_less(n, ds)) {
			n = wide_sub(n, ds);
			q.lo |= 1ULL;
		}
		if (shift == 0)
			break;
		ds = wide_shr1(ds);
		shift--;
	}
	if (rem != 0)
		*rem = n;
	return q;
}

libvcc_u128 __udivti3(libvcc_u128 n, libvcc_u128 d)
{
	libvcc_u128 rem;

	return __udivmodti4(n, d, &rem);
}

libvcc_u128 __umodti3(libvcc_u128 n, libvcc_u128 d)
{
	libvcc_u128 rem;

	(void)__udivmodti4(n, d, &rem);
	return rem;
}

/*
 * The sign is taken off in unsigned arithmetic, so the most negative value
 * negates without overflowing. Two's complement is the low word plus the high
 * word, so the cast back is what the ABI means and not a reinterpretation.
 *
 * The quotient of the most negative value by -1 is not representable and the
 * ABI does not define it; the answer here is the wrapped one, which is what
 * libgcc answers too.
 */

static int wide_negative(libvcc_u128 v)
{
	return (v.hi & 0x8000000000000000ULL) != 0;
}

static libvcc_u128 wide_neg(libvcc_u128 v)
{
	libvcc_u128 r;

	r.lo = 0ULL - v.lo;
	r.hi = ~v.hi + (v.lo == 0 ? 1ULL : 0ULL);
	return r;
}

libvcc_u128 __divmodti4(libvcc_u128 a, libvcc_u128 b, libvcc_u128 *rem)
{
	libvcc_u128 q, r, ua, ub;
	int neg_q, neg_r;

	ua = a;
	ub = b;
	neg_q = wide_negative(a) != wide_negative(b);
	neg_r = wide_negative(a);
	if (wide_negative(a))
		ua = wide_neg(ua);
	if (wide_negative(b))
		ub = wide_neg(ub);
	q = __udivmodti4(ua, ub, &r);
	if (neg_q)
		q = wide_neg(q);
	if (neg_r)
		r = wide_neg(r);
	if (rem != 0)
		*rem = r;
	return q;
}

libvcc_u128 __divti3(libvcc_u128 a, libvcc_u128 b)
{
	libvcc_u128 rem;

	return __divmodti4(a, b, &rem);
}

libvcc_u128 __modti3(libvcc_u128 a, libvcc_u128 b)
{
	libvcc_u128 rem;

	(void)__divmodti4(a, b, &rem);
	return rem;
}
