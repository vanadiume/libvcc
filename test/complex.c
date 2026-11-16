#include <libvcc/builtins.h>
#include <libvcc/compiler.h>

#include "harness.h"

/*
 * The values the complex helpers answer.
 *
 * Every expected value here is a literal recorded from gcc 16.2.1's own
 * helper, not from the formula the helper carries:
 *
 *   gcc -std=gnu89 -O0 -fno-builtin record.c -o record
 *
 * where record.c declares the six symbols and prints each quotient and
 * product with %.17g for double, %.9g for float and %.21Lg for long double.
 * Those are the digits that read back as the same value. The comparison below
 * is bit for bit, so a sign of zero counts as well as a different number.
 *
 * The cases are the ones where the arithmetic has to be gcc's rather than the
 * obvious (a*c + b*d) / (c*c + d*d): a divisor whose component is at the top
 * of the range, where c*c overflows; one at the bottom, where c*c underflows
 * to zero; a ratio small enough that the products have to be formed another
 * way; and a zero divisor, where the quotient is the infinity the operands
 * imply. vcc's inline division answers that last one with NaN (a program it
 * compiles prints -nan -nan for (1e308 + 1e308i) / (0 + 0i), where gcc prints
 * inf inf), which is why the helper has to be asked rather than the operator.
 *
 * The long double cases are read back through the union the helpers use, and
 * vcc cannot yet read a value of that type back at all; compiler.h says why,
 * and the gate keeps those checks out of its lane rather than pretending they
 * ran.
 */

#define CHECK_D(got, want) \
	do { \
		union { double d; unsigned long long u; } g_, w_; \
		g_.d = (got); \
		w_.d = (want); \
		CHECK_U64(g_.u, w_.u); \
	} while (0)

#define CHECK_F(got, want) \
	do { \
		union { float f; unsigned int u; } g_, w_; \
		g_.f = (got); \
		w_.f = (want); \
		CHECK_U64(g_.u, w_.u); \
	} while (0)

#define CHECK_X(got, want) \
	do { \
		union { long double d; unsigned short w[8]; } g_, w_; \
		int k_; \
		g_.d = (got); \
		w_.d = (want); \
		/* Ten bytes are the value; the six after them are padding a
		   compiler need not copy. */ \
		for (k_ = 0; k_ < 5; k_++) \
			CHECK_U64(g_.w[k_], w_.w[k_]); \
	} while (0)

#define INF_D (1.0 / 0.0)
#define INF_F (1.0f / 0.0f)
#define INF_X (1.0L / 0.0L)

void libvcc_test_complex(void)
{
	double _Complex d;
	float _Complex f;
	union {
		long double _Complex z;
		struct {
			long double r, i;
		} parts;
	} x;

	/* (1 + 2i) / (3 + 4i) = 11/25 + 2/25 i, the case with no scaling. */
	d = __divdc3(1.0, 2.0, 3.0, 4.0);
	CHECK_D(__real__ d, 0.44);
	CHECK_D(__imag__ d, 0.080000000000000002);

	/* A divisor at the top of the range: c*c would overflow. A value over
	   itself is one, however large it is. */
	d = __divdc3(1e308, 1e308, 1e308, 1e308);
	CHECK_D(__real__ d, 1.0);
	CHECK_D(__imag__ d, 0.0);

	/* A divisor at the bottom: c*c would underflow to zero, and the
	   operands are scaled up before the ratio is formed. */
	d = __divdc3(1e-308, 1e-308, 1e-308, 1e-308);
	CHECK_D(__real__ d, 1.0);
	CHECK_D(__imag__ d, 0.0);

	/* The quotient of two tiny values by a normal one, which takes the
	   scale-up branch rather than the ratio branch. */
	d = __divdc3(1e-308, 1e-308, 3.0, 0.0);
	CHECK_D(__real__ d, 3.3333333333333314e-309);
	CHECK_D(__imag__ d, 3.3333333333333314e-309);

	/* A ratio below the smallest normal, where the products are formed
	   from a divided operand instead. */
	d = __divdc3(1.0, 1.0, 1e-308, 3.0);
	CHECK_D(__real__ d, 0.33333333333333331);
	CHECK_D(__imag__ d, -0.33333333333333331);

	/* Division by i. */
	d = __divdc3(3.0, 4.0, 0.0, 1.0);
	CHECK_D(__real__ d, 4.0);
	CHECK_D(__imag__ d, -3.0);

	/* Division by zero, which is the case vcc's inline code answers with
	   NaN: a nonzero divided by zero is infinite. */
	d = __divdc3(1e308, 1e308, 0.0, 0.0);
	CHECK_D(__real__ d, INF_D);
	CHECK_D(__imag__ d, INF_D);

	/* (1 + 2i) * (3 + 4i) = -5 + 10i. */
	d = __muldc3(1.0, 2.0, 3.0, 4.0);
	CHECK_D(__real__ d, -5.0);
	CHECK_D(__imag__ d, 10.0);

	/* A product that overflows, where the quotient's recovery has no part
	   to play: the components are the infinities the arithmetic gives. */
	d = __muldc3(1e200, 1e200, 1e200, 0.0);
	CHECK_D(__real__ d, INF_D);
	CHECK_D(__imag__ d, INF_D);

	/* The same three shapes at float width, where the division is done in
	   double and each component is rounded back. */
	f = __divsc3(1.0f, 2.0f, 3.0f, 4.0f);
	CHECK_F(__real__ f, 0.439999998f);
	CHECK_F(__imag__ f, 0.0799999982f);

	f = __divsc3(1e-38f, 1e-38f, 3.0f, 0.0f);
	CHECK_F(__real__ f, 3.33333312e-39f);
	CHECK_F(__imag__ f, 3.33333312e-39f);

	f = __divsc3(3.0f, 4.0f, 0.0f, 1.0f);
	CHECK_F(__real__ f, 4.0f);
	CHECK_F(__imag__ f, -3.0f);

	f = __divsc3(1e38f, 1e38f, 0.0f, 0.0f);
	CHECK_F(__real__ f, INF_F);
	CHECK_F(__imag__ f, INF_F);

	f = __mulsc3(1.0f, 2.0f, 3.0f, 4.0f);
	CHECK_F(__real__ f, -5.0f);
	CHECK_F(__imag__ f, 10.0f);

	/* The extended width, where the components are the x87 stack's. */
	x.z = __divxc3(1.0L, 2.0L, 3.0L, 4.0L);
	CHECK_X(x.parts.r, 0.439999999999999999998L);
	CHECK_X(x.parts.i, 0.0799999999999999999984L);

	x.z = __divxc3(1e4932L, 1e4932L, 1e4932L, 1e4932L);
	CHECK_X(x.parts.r, 1.0L);
	CHECK_X(x.parts.i, 0.0L);

	x.z = __divxc3(3.0L, 4.0L, 0.0L, 1.0L);
	CHECK_X(x.parts.r, 4.0L);
	CHECK_X(x.parts.i, -3.0L);

	x.z = __divxc3(1e4932L, 1e4932L, 0.0L, 0.0L);
	CHECK_X(x.parts.r, INF_X);
	CHECK_X(x.parts.i, INF_X);

	x.z = __mulxc3(1.0L, 2.0L, 3.0L, 4.0L);
	CHECK_X(x.parts.r, -5.0L);
	CHECK_X(x.parts.i, 10.0L);
}
