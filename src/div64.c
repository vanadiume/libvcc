#include <libvcc/builtins.h>

/*
 * Restoring division, one quotient bit at a time, with the divisor shifted so
 * that its top set bit lines up with the dividend's. The shift count is what
 * keeps this from overflowing: d << shift is at most n, so the top bit of d
 * never leaves the word.
 *
 * Nothing here uses a 64-bit division. That is the whole point of the file.
 */

unsigned long long __udivmoddi4(unsigned long long n, unsigned long long d,
                                unsigned long long *rem)
{
	unsigned long long q;
	int shift;

	if (d == 0) {
		if (rem != 0)
			*rem = n;
		return 0;
	}
	if (n < d) {
		if (rem != 0)
			*rem = n;
		return 0;
	}
	shift = __clzdi2(d) - __clzdi2(n);
	d <<= shift;
	q = 0;
	for (;;) {
		q <<= 1;
		if (n >= d) {
			n -= d;
			q |= 1;
		}
		if (shift == 0)
			break;
		d >>= 1;
		shift--;
	}
	if (rem != 0)
		*rem = n;
	return q;
}

unsigned long long __udivdi3(unsigned long long n, unsigned long long d)
{
	unsigned long long rem;

	return __udivmoddi4(n, d, &rem);
}

unsigned long long __umoddi3(unsigned long long n, unsigned long long d)
{
	unsigned long long rem;

	(void)__udivmoddi4(n, d, &rem);
	return rem;
}

/*
 * The sign is taken off in unsigned arithmetic, so the most negative value
 * negates without overflowing. The cast back to long long is the ABI's
 * two's-complement answer; every compiler in the matrix does that.
 */

long long __divdi3(long long a, long long b)
{
	unsigned long long ua, ub, uq, ur;
	int neg;

	ua = (unsigned long long)a;
	ub = (unsigned long long)b;
	neg = (a < 0) != (b < 0);
	if (a < 0)
		ua = 0 - ua;
	if (b < 0)
		ub = 0 - ub;
	uq = __udivmoddi4(ua, ub, &ur);
	if (neg)
		uq = 0 - uq;
	return (long long)uq;
}

long long __moddi3(long long a, long long b)
{
	unsigned long long ua, ub, ur;
	int neg;

	ua = (unsigned long long)a;
	ub = (unsigned long long)b;
	neg = a < 0;
	if (a < 0)
		ua = 0 - ua;
	if (b < 0)
		ub = 0 - ub;
	(void)__udivmoddi4(ua, ub, &ur);
	if (neg)
		ur = 0 - ur;
	return (long long)ur;
}
