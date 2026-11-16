#include <libvcc/compiler.h>

#include "harness.h"

/*
 * The caller's side of the complex ABI.
 *
 * The declarations below spell the six helpers with the compiler's own complex
 * type, as a program that divides two complex values does, and each call is
 * compared with the operator on the same operands. What that measures depends
 * on the compiler: gcc 16.2.1 folds a product or a quotient of constants into
 * the result even at -O0, so there the comparison is against its arithmetic;
 * vcc computes the quotient and the product inline; clang 23.1.4 emits the
 * call at -O0, so both sides are the library and what the file shows in that
 * lane is that the call site the compiler builds and the definition agree.
 *
 * The value travels differently for each width. A double component pair comes
 * back in xmm0 and xmm1, a float pair packed in xmm0, and a long double pair on
 * the x87 stack, st0 and st1. If the definition put any of them anywhere else,
 * these calls would read whatever was in the register or slot instead.
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

extern double _Complex __muldc3(double a, double b, double c, double d);
extern double _Complex __divdc3(double a, double b, double c, double d);
extern float _Complex __mulsc3(float a, float b, float c, float d);
extern float _Complex __divsc3(float a, float b, float c, float d);
extern long double _Complex __mulxc3(long double a, long double b,
                                     long double c, long double d);
extern long double _Complex __divxc3(long double a, long double b,
                                     long double c, long double d);

void libvcc_test_abi_complex(void)
{
	double _Complex d, wantd;
	float _Complex f, wantf;
	long double _Complex x, wantx;

	d = __muldc3(1.0, 2.0, 3.0, 4.0);
	wantd = (1.0 + 2.0i) * (3.0 + 4.0i);
	CHECK_D(__real__ d, __real__ wantd);
	CHECK_D(__imag__ d, __imag__ wantd);

	d = __divdc3(1.0, 2.0, 3.0, 4.0);
	wantd = (1.0 + 2.0i) / (3.0 + 4.0i);
	CHECK_D(__real__ d, __real__ wantd);
	CHECK_D(__imag__ d, __imag__ wantd);

	f = __mulsc3(1.0f, 2.0f, 3.0f, 4.0f);
	wantf = (1.0f + 2.0fi) * (3.0f + 4.0fi);
	CHECK_F(__real__ f, __real__ wantf);
	CHECK_F(__imag__ f, __imag__ wantf);

	f = __divsc3(1.0f, 2.0f, 3.0f, 4.0f);
	wantf = (1.0f + 2.0fi) / (3.0f + 4.0fi);
	CHECK_F(__real__ f, __real__ wantf);
	CHECK_F(__imag__ f, __imag__ wantf);

	x = __mulxc3(1.0L, 2.0L, 3.0L, 4.0L);
	wantx = (1.0L + 2.0Li) * (3.0L + 4.0Li);
	CHECK(__real__ x == __real__ wantx);
	CHECK(__imag__ x == __imag__ wantx);

	x = __divxc3(1.0L, 2.0L, 3.0L, 4.0L);
	wantx = (1.0L + 2.0Li) / (3.0L + 4.0Li);
	CHECK(__real__ x == __real__ wantx);
	CHECK(__imag__ x == __imag__ wantx);
}
