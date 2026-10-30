#include <libvcc/builtins.h>

/*
 * The same division as div64.c, one word wide. A 32-bit target has no call to
 * make for this, but the pair is part of the ABI and the algorithm is the one
 * already tested, so it is here rather than arriving with a second target.
 */

unsigned int __udivmodsi4(unsigned int n, unsigned int d, unsigned int *rem)
{
	unsigned int q;
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
	shift = __clzsi2(d) - __clzsi2(n);
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

unsigned int __udivsi3(unsigned int n, unsigned int d)
{
	unsigned int rem;

	return __udivmodsi4(n, d, &rem);
}

unsigned int __umodsi3(unsigned int n, unsigned int d)
{
	unsigned int rem;

	(void)__udivmodsi4(n, d, &rem);
	return rem;
}

int __divsi3(int a, int b)
{
	unsigned int ua, ub, uq, ur;
	int neg;

	ua = (unsigned int)a;
	ub = (unsigned int)b;
	neg = (a < 0) != (b < 0);
	if (a < 0)
		ua = 0 - ua;
	if (b < 0)
		ub = 0 - ub;
	uq = __udivmodsi4(ua, ub, &ur);
	if (neg)
		uq = 0 - uq;
	return (int)uq;
}

int __modsi3(int a, int b)
{
	unsigned int ua, ub, ur;
	int neg;

	ua = (unsigned int)a;
	ub = (unsigned int)b;
	neg = a < 0;
	if (a < 0)
		ua = 0 - ua;
	if (b < 0)
		ub = 0 - ub;
	(void)__udivmodsi4(ua, ub, &ur);
	if (neg)
		ur = 0 - ur;
	return (int)ur;
}
