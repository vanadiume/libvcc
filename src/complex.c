#include <libvcc/builtins.h>

/*
 * Complex multiply and divide.
 *
 * A compiler with no complex instruction emits __muldc3 and __divdc3 for
 * `a * b` and `a / b` on `double _Complex`, and the sc and xc spellings for
 * the other two widths. The arguments are the four scalars in the order the
 * source wrote them, not two complex objects, and the value comes back in the
 * floating-point registers a complex value uses: xmm0 and xmm1 for double,
 * xmm0 packed for float, st0 and st1 for long double.
 *
 * The arithmetic is gcc 16.2.1's libgcc rather than the formula C99's Annex G
 * sketches. The sketch, (a*c + b*d) / (c*c + d*d), overflows as soon as a
 * component of the divisor is large, so the helper scales its operands first,
 * and when the quotient still computed as NaN + NaN i it recovers the
 * infinities and zeros the operands imply: a nonzero divided by zero is
 * infinite, an infinite divided by a finite is infinite, and a finite divided
 * by an infinite is zero. vcc's inline division does the scaling and stops
 * there, which is why `(1e308 + 1e308i) / (0 + 0i)` answers -nan with it and
 * inf with this library.
 *
 * gcc's helper contracts its products and sums into fused multiply-adds,
 * because libgcc is built with contraction on. This source names the same
 * products and sums and leaves the contraction to the compiler, which for the
 * x86-64 target builds none: a product and the sum it feeds round twice.
 * Measured with a harness gcc -O2 -std=gnu89 compiled, which called gcc
 * 16.2.1's __divdc3 and this one over 83521 quadruples of seventeen extremes in
 * each component and compared the components as bits: the double quotient lands
 * one ulp away on 549 of them, and on none when the harness is built with
 * -mfma. The product, the float helpers and the long double helpers agree on
 * all of them either way.
 */

/*
 * The two components of a complex value as scalars.
 *
 * The store is written through the struct: the compiler this library exists
 * for has no store that writes one part of a complex value, and refuses
 * `__real__ r = ...` for saying so. The return is the complex member, which is
 * the object the caller is handed. The struct is not the return type on its
 * own: a 32-byte struct return travels behind a hidden pointer, while
 * `long double _Complex` is handed back on the x87 stack, so one spelling of
 * the union covers the three widths.
 */
union libvcc_double_complex {
	double _Complex z;
	struct {
		double r;
		double i;
	} parts;
};

union libvcc_float_complex {
	float _Complex z;
	struct {
		float r;
		float i;
	} parts;
};

union libvcc_long_double_complex {
	long double _Complex z;
	struct {
		long double r;
		long double i;
	} parts;
};

/* The same object seen as bits, for the sign the recovery copies from one
   value to another. A NaN carries a sign bit no comparison reports. */
union libvcc_double_bits {
	double d;
	unsigned long long u;
};

union libvcc_float_bits {
	float f;
	unsigned int u;
};

/* An extended-precision value is sixteen bytes of which ten are used: the
   significand's eight, then the sign and exponent in the two after them, and
   six of padding. The words name that layout, which is x86-64's. */
union libvcc_long_double_bits {
	long double d;
	unsigned short w[8];
};

/* Infinity and zero, built from arithmetic and from a sign bit so that
   nothing here names a symbol: a zero the compiler may not fold gives
   infinity, and a mask leaves a zero carrying a value's sign. */
static double libvcc_infinity_d(void)
{
	volatile double zero;

	zero = 0.0;
	return 1.0 / zero;
}

static float libvcc_infinity_f(void)
{
	volatile float zero;

	zero = 0.0f;
	return 1.0f / zero;
}

static long double libvcc_infinity_x(void)
{
	volatile long double zero;

	zero = 0.0L;
	return 1.0L / zero;
}

static double libvcc_mag_d(double x)
{
	union libvcc_double_bits bits;

	bits.d = x;
	bits.u = bits.u & 0x7fffffffffffffffULL;
	return bits.d;
}

static long double libvcc_mag_x(long double x)
{
	union libvcc_long_double_bits bits;

	bits.d = x;
	bits.w[4] = bits.w[4] & 0x7fffU;
	return bits.d;
}

static int libvcc_isinf_d(double x, double inf)
{
	return x == inf || x == -inf;
}

static int libvcc_isinf_f(float x, float inf)
{
	return x == inf || x == -inf;
}

static int libvcc_isinf_x(long double x, long double inf)
{
	return x == inf || x == -inf;
}

static double libvcc_zero_with_sign_d(double value)
{
	union libvcc_double_bits bits;

	bits.d = value;
	bits.u = bits.u & 0x8000000000000000ULL;
	return bits.d;
}

static float libvcc_zero_with_sign_f(float value)
{
	union libvcc_float_bits bits;

	bits.f = value;
	bits.u = bits.u & 0x80000000U;
	return bits.f;
}

static long double libvcc_zero_with_sign_x(long double value)
{
	union libvcc_long_double_bits bits;
	unsigned short sign;
	int k;

	bits.d = value;
	sign = bits.w[4] & 0x8000U;
	for (k = 0; k < 8; k++)
		bits.w[k] = 0;
	bits.w[4] = sign;
	return bits.d;
}

/* What gcc's COPYSIGN (isinf (x) ? 1 : 0, x) is worth: the sign of an
   infinity as a unit, and a zero carrying the sign of anything else. */
static double libvcc_unit_d(double x, double inf)
{
	if (x == inf)
		return 1.0;
	if (x == -inf)
		return -1.0;
	return libvcc_zero_with_sign_d(x);
}

static float libvcc_unit_f(float x, float inf)
{
	if (x == inf)
		return 1.0f;
	if (x == -inf)
		return -1.0f;
	return libvcc_zero_with_sign_f(x);
}

static long double libvcc_unit_x(long double x, long double inf)
{
	if (x == inf)
		return 1.0L;
	if (x == -inf)
		return -1.0L;
	return libvcc_zero_with_sign_x(x);
}

double _Complex __muldc3(double a, double b, double c, double d)
{
	double ac, bd, ad, bc, x, y, inf;
	int recalc;
	union libvcc_double_complex out;

	ac = a * c;
	bd = b * d;
	ad = a * d;
	bc = b * c;
	x = ac - bd;
	y = ad + bc;

	if ((x != x) && (y != y)) {
		/* Recover the infinities that computed as NaN + NaN i. */
		inf = libvcc_infinity_d();
		recalc = 0;
		if (libvcc_isinf_d(a, inf) || libvcc_isinf_d(b, inf)) {
			/* z is infinite: box the infinity and read a NaN in
			   the other factor as a zero. */
			a = libvcc_unit_d(a, inf);
			b = libvcc_unit_d(b, inf);
			if (c != c)
				c = libvcc_zero_with_sign_d(c);
			if (d != d)
				d = libvcc_zero_with_sign_d(d);
			recalc = 1;
		}
		if (libvcc_isinf_d(c, inf) || libvcc_isinf_d(d, inf)) {
			c = libvcc_unit_d(c, inf);
			d = libvcc_unit_d(d, inf);
			if (a != a)
				a = libvcc_zero_with_sign_d(a);
			if (b != b)
				b = libvcc_zero_with_sign_d(b);
			recalc = 1;
		}
		if (!recalc && (libvcc_isinf_d(ac, inf) || libvcc_isinf_d(bd, inf)
		                || libvcc_isinf_d(ad, inf)
		                || libvcc_isinf_d(bc, inf))) {
			/* Recover infinities that came from overflow. */
			if (a != a)
				a = libvcc_zero_with_sign_d(a);
			if (b != b)
				b = libvcc_zero_with_sign_d(b);
			if (c != c)
				c = libvcc_zero_with_sign_d(c);
			if (d != d)
				d = libvcc_zero_with_sign_d(d);
			recalc = 1;
		}
		if (recalc) {
			x = inf * ((a * c) - (b * d));
			y = inf * ((a * d) + (b * c));
		}
	}

	out.parts.r = x;
	out.parts.i = y;
	return out.z;
}

float _Complex __mulsc3(float a, float b, float c, float d)
{
	float ac, bd, ad, bc, x, y, inf;
	int recalc;
	union libvcc_float_complex out;

	ac = a * c;
	bd = b * d;
	ad = a * d;
	bc = b * c;
	x = ac - bd;
	y = ad + bc;

	if ((x != x) && (y != y)) {
		inf = libvcc_infinity_f();
		recalc = 0;
		if (libvcc_isinf_f(a, inf) || libvcc_isinf_f(b, inf)) {
			a = libvcc_unit_f(a, inf);
			b = libvcc_unit_f(b, inf);
			if (c != c)
				c = libvcc_zero_with_sign_f(c);
			if (d != d)
				d = libvcc_zero_with_sign_f(d);
			recalc = 1;
		}
		if (libvcc_isinf_f(c, inf) || libvcc_isinf_f(d, inf)) {
			c = libvcc_unit_f(c, inf);
			d = libvcc_unit_f(d, inf);
			if (a != a)
				a = libvcc_zero_with_sign_f(a);
			if (b != b)
				b = libvcc_zero_with_sign_f(b);
			recalc = 1;
		}
		if (!recalc && (libvcc_isinf_f(ac, inf) || libvcc_isinf_f(bd, inf)
		                || libvcc_isinf_f(ad, inf)
		                || libvcc_isinf_f(bc, inf))) {
			if (a != a)
				a = libvcc_zero_with_sign_f(a);
			if (b != b)
				b = libvcc_zero_with_sign_f(b);
			if (c != c)
				c = libvcc_zero_with_sign_f(c);
			if (d != d)
				d = libvcc_zero_with_sign_f(d);
			recalc = 1;
		}
		if (recalc) {
			x = inf * ((a * c) - (b * d));
			y = inf * ((a * d) + (b * c));
		}
	}

	out.parts.r = x;
	out.parts.i = y;
	return out.z;
}

long double _Complex __mulxc3(long double a, long double b, long double c,
                              long double d)
{
	long double ac, bd, ad, bc, x, y, inf;
	int recalc;
	union libvcc_long_double_complex out;

	ac = a * c;
	bd = b * d;
	ad = a * d;
	bc = b * c;
	x = ac - bd;
	y = ad + bc;

	if ((x != x) && (y != y)) {
		inf = libvcc_infinity_x();
		recalc = 0;
		if (libvcc_isinf_x(a, inf) || libvcc_isinf_x(b, inf)) {
			a = libvcc_unit_x(a, inf);
			b = libvcc_unit_x(b, inf);
			if (c != c)
				c = libvcc_zero_with_sign_x(c);
			if (d != d)
				d = libvcc_zero_with_sign_x(d);
			recalc = 1;
		}
		if (libvcc_isinf_x(c, inf) || libvcc_isinf_x(d, inf)) {
			c = libvcc_unit_x(c, inf);
			d = libvcc_unit_x(d, inf);
			if (a != a)
				a = libvcc_zero_with_sign_x(a);
			if (b != b)
				b = libvcc_zero_with_sign_x(b);
			recalc = 1;
		}
		if (!recalc && (libvcc_isinf_x(ac, inf) || libvcc_isinf_x(bd, inf)
		                || libvcc_isinf_x(ad, inf)
		                || libvcc_isinf_x(bc, inf))) {
			if (a != a)
				a = libvcc_zero_with_sign_x(a);
			if (b != b)
				b = libvcc_zero_with_sign_x(b);
			if (c != c)
				c = libvcc_zero_with_sign_x(c);
			if (d != d)
				d = libvcc_zero_with_sign_x(d);
			recalc = 1;
		}
		if (recalc) {
			x = inf * ((a * c) - (b * d));
			y = inf * ((a * d) + (b * c));
		}
	}

	out.parts.r = x;
	out.parts.i = y;
	return out.z;
}

double _Complex __divdc3(double a, double b, double c, double d)
{
	/* Half the largest finite double, the smallest normal, the machine
	   epsilon, its reciprocal, and the product of the first and third. */
	const double rbig = 8.9884656743115785e307;
	const double rmin = 2.2250738585072014e-308;
	const double rmin2 = 2.2204460492503131e-16;
	const double rmax2 = 1.9958403095347196e292;
	const double rminscal = 4503599627370496.0;
	double denom, ratio, x, y, inf;
	union libvcc_double_complex out;

	/* Scale by the larger component of the divisor, and scale both operands
	   up when either one is small enough that the quotient could underflow
	   on the way. */
	if (libvcc_mag_d(c) < libvcc_mag_d(d)) {
		if (libvcc_mag_d(d) >= rbig) {
			a = a / 2.0;
			b = b / 2.0;
			c = c / 2.0;
			d = d / 2.0;
		}
		if (libvcc_mag_d(d) < rmin2) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		} else if (((libvcc_mag_d(a) < rmin)
		            && (libvcc_mag_d(b) < rmax2)
		            && (libvcc_mag_d(d) < rmax2))
		           || ((libvcc_mag_d(b) < rmin)
		               && (libvcc_mag_d(a) < rmax2)
		               && (libvcc_mag_d(d) < rmax2))) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		}
		ratio = c / d;
		denom = (c * ratio) + d;
		/* A subnormal ratio loses the products, so form them from a
		   divided operand instead. */
		if (libvcc_mag_d(ratio) > rmin) {
			x = ((a * ratio) + b) / denom;
			y = ((b * ratio) - a) / denom;
		} else {
			x = ((c * (a / d)) + b) / denom;
			y = ((c * (b / d)) - a) / denom;
		}
	} else {
		if (libvcc_mag_d(c) >= rbig) {
			a = a / 2.0;
			b = b / 2.0;
			c = c / 2.0;
			d = d / 2.0;
		}
		if (libvcc_mag_d(c) < rmin2) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		} else if (((libvcc_mag_d(a) < rmin)
		            && (libvcc_mag_d(b) < rmax2)
		            && (libvcc_mag_d(c) < rmax2))
		           || ((libvcc_mag_d(b) < rmin)
		               && (libvcc_mag_d(a) < rmax2)
		               && (libvcc_mag_d(c) < rmax2))) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		}
		ratio = d / c;
		denom = (d * ratio) + c;
		if (libvcc_mag_d(ratio) > rmin) {
			x = ((b * ratio) + a) / denom;
			y = (b - (a * ratio)) / denom;
		} else {
			x = (a + (d * (b / c))) / denom;
			y = (b - (d * (a / c))) / denom;
		}
	}

	if ((x != x) && (y != y)) {
		/* Recover the infinities and zeros that computed as NaN + NaN i.
		   A nonzero divided by zero is infinite, an infinity over a
		   finite value is infinite, and a finite value over an infinity
		   is zero. */
		inf = libvcc_infinity_d();
		if ((c == 0.0) && (d == 0.0) && !((a != a) && (b != b))) {
			x = (inf / c) * a;
			y = (inf / c) * b;
		} else if ((libvcc_isinf_d(a, inf) || libvcc_isinf_d(b, inf))
		           && !libvcc_isinf_d(c, inf) && (c == c)
		           && !libvcc_isinf_d(d, inf) && (d == d)) {
			a = libvcc_unit_d(a, inf);
			b = libvcc_unit_d(b, inf);
			x = inf * ((a * c) + (b * d));
			y = inf * ((b * c) - (a * d));
		} else if ((libvcc_isinf_d(c, inf) || libvcc_isinf_d(d, inf))
		           && !libvcc_isinf_d(a, inf) && (a == a)
		           && !libvcc_isinf_d(b, inf) && (b == b)) {
			c = libvcc_unit_d(c, inf);
			d = libvcc_unit_d(d, inf);
			x = 0.0 * ((a * c) + (b * d));
			y = 0.0 * ((b * c) - (a * d));
		}
	}

	out.parts.r = x;
	out.parts.i = y;
	return out.z;
}

/*
 * float division does not scale. gcc's __divsc3 widens both operands to double,
 * divides there with the plain formula, and rounds each component back to a
 * float, and the extra precision is what makes the formula enough.
 */
float _Complex __divsc3(float a, float b, float c, float d)
{
	double aa, bb, cc, dd, denom;
	float x, y, inf;
	union libvcc_float_complex out;

	aa = a;
	bb = b;
	cc = c;
	dd = d;
	denom = (cc * cc) + (dd * dd);
	x = (float)(((aa * cc) + (bb * dd)) / denom);
	y = (float)(((bb * cc) - (aa * dd)) / denom);

	if ((x != x) && (y != y)) {
		inf = libvcc_infinity_f();
		if ((c == 0.0f) && (d == 0.0f) && !((a != a) && (b != b))) {
			x = (inf / c) * a;
			y = (inf / c) * b;
		} else if ((libvcc_isinf_f(a, inf) || libvcc_isinf_f(b, inf))
		           && !libvcc_isinf_f(c, inf) && (c == c)
		           && !libvcc_isinf_f(d, inf) && (d == d)) {
			a = libvcc_unit_f(a, inf);
			b = libvcc_unit_f(b, inf);
			x = inf * ((a * c) + (b * d));
			y = inf * ((b * c) - (a * d));
		} else if ((libvcc_isinf_f(c, inf) || libvcc_isinf_f(d, inf))
		           && !libvcc_isinf_f(a, inf) && (a == a)
		           && !libvcc_isinf_f(b, inf) && (b == b)) {
			c = libvcc_unit_f(c, inf);
			d = libvcc_unit_f(d, inf);
			x = 0.0f * ((a * c) + (b * d));
			y = 0.0f * ((b * c) - (a * d));
		}
	}

	out.parts.r = x;
	out.parts.i = y;
	return out.z;
}

long double _Complex __divxc3(long double a, long double b, long double c,
                              long double d)
{
	const long double rbig = 5.948657476786158825106319e+4931L;
	const long double rmin = 3.362103143112093506262678e-4932L;
	const long double rmin2 = 1.084202172485504434007453e-19L;
	const long double rmax2 = 6.449547359703692559088506e+4912L;
	const long double rminscal = 9223372036854775808.0L;
	long double denom, ratio, x, y, inf;
	union libvcc_long_double_complex out;

	if (libvcc_mag_x(c) < libvcc_mag_x(d)) {
		if (libvcc_mag_x(d) >= rbig) {
			a = a / 2.0L;
			b = b / 2.0L;
			c = c / 2.0L;
			d = d / 2.0L;
		}
		if (libvcc_mag_x(d) < rmin2) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		} else if (((libvcc_mag_x(a) < rmin)
		            && (libvcc_mag_x(b) < rmax2)
		            && (libvcc_mag_x(d) < rmax2))
		           || ((libvcc_mag_x(b) < rmin)
		               && (libvcc_mag_x(a) < rmax2)
		               && (libvcc_mag_x(d) < rmax2))) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		}
		ratio = c / d;
		denom = (c * ratio) + d;
		if (libvcc_mag_x(ratio) > rmin) {
			x = ((a * ratio) + b) / denom;
			y = ((b * ratio) - a) / denom;
		} else {
			x = ((c * (a / d)) + b) / denom;
			y = ((c * (b / d)) - a) / denom;
		}
	} else {
		if (libvcc_mag_x(c) >= rbig) {
			a = a / 2.0L;
			b = b / 2.0L;
			c = c / 2.0L;
			d = d / 2.0L;
		}
		if (libvcc_mag_x(c) < rmin2) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		} else if (((libvcc_mag_x(a) < rmin)
		            && (libvcc_mag_x(b) < rmax2)
		            && (libvcc_mag_x(c) < rmax2))
		           || ((libvcc_mag_x(b) < rmin)
		               && (libvcc_mag_x(a) < rmax2)
		               && (libvcc_mag_x(c) < rmax2))) {
			a = a * rminscal;
			b = b * rminscal;
			c = c * rminscal;
			d = d * rminscal;
		}
		ratio = d / c;
		denom = (d * ratio) + c;
		if (libvcc_mag_x(ratio) > rmin) {
			x = ((b * ratio) + a) / denom;
			y = (b - (a * ratio)) / denom;
		} else {
			x = (a + (d * (b / c))) / denom;
			y = (b - (d * (a / c))) / denom;
		}
	}

	if ((x != x) && (y != y)) {
		inf = libvcc_infinity_x();
		if ((c == 0.0L) && (d == 0.0L) && !((a != a) && (b != b))) {
			x = (inf / c) * a;
			y = (inf / c) * b;
		} else if ((libvcc_isinf_x(a, inf) || libvcc_isinf_x(b, inf))
		           && !libvcc_isinf_x(c, inf) && (c == c)
		           && !libvcc_isinf_x(d, inf) && (d == d)) {
			a = libvcc_unit_x(a, inf);
			b = libvcc_unit_x(b, inf);
			x = inf * ((a * c) + (b * d));
			y = inf * ((b * c) - (a * d));
		} else if ((libvcc_isinf_x(c, inf) || libvcc_isinf_x(d, inf))
		           && !libvcc_isinf_x(a, inf) && (a == a)
		           && !libvcc_isinf_x(b, inf) && (b == b)) {
			c = libvcc_unit_x(c, inf);
			d = libvcc_unit_x(d, inf);
			x = 0.0L * ((a * c) + (b * d));
			y = 0.0L * ((b * c) - (a * d));
		}
	}

	out.parts.r = x;
	out.parts.i = y;
	return out.z;
}
