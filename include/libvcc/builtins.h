#ifndef LIBVCC_BUILTINS_H
#define LIBVCC_BUILTINS_H

/*
 * The helper ABI.
 *
 * These are libgcc's names on purpose. A compiler emits a call to __divdi3
 * because that is the name it was taught, and a runtime library that renamed
 * it would not be a runtime library. Link libvcc.a instead of libgcc.a, not
 * beside it: the two define the same symbols and the first one the linker
 * finds wins.
 *
 * The types are the C types, not typedefs of libvcc's own. long long is 64
 * bits and int is 32 on every target this is built for, which is the same
 * assumption libgcc makes.
 *
 * The library calls nothing. There is no libc here, no abort, no memcpy, and
 * no header from the system; a helper that cannot do its work with the
 * language gets its own file and its own discussion rather than a dependency
 * added to the archive.
 *
 * Division by zero is not defined by the ABI. __udivmoddi4,
 * __udivmodsi4 and __udivmodti4 answer zero with the remainder set to the
 * dividend, and __divmodti4 does the same for a signed pair, so a caller that
 * divides by zero gets a wrong number and not a trap. A compiler is expected
 * to have diagnosed the constant case before it emitted a call.
 */

extern long long __divdi3(long long a, long long b);
extern long long __moddi3(long long a, long long b);
extern unsigned long long __udivdi3(unsigned long long n, unsigned long long d);
extern unsigned long long __umoddi3(unsigned long long n, unsigned long long d);
extern unsigned long long __udivmoddi4(unsigned long long n,
                                       unsigned long long d,
                                       unsigned long long *rem);

extern int __divsi3(int a, int b);
extern int __modsi3(int a, int b);
extern unsigned int __udivsi3(unsigned int n, unsigned int d);
extern unsigned int __umodsi3(unsigned int n, unsigned int d);
extern unsigned int __udivmodsi4(unsigned int n, unsigned int d,
                                 unsigned int *rem);

/* Counts are 0 through 63 for the 64-bit helpers and 0 through 31 for the
   32-bit ones. Anything else is outside the ABI and not tested. */
extern long long __ashldi3(long long a, int b);
extern long long __ashrdi3(long long a, int b);
extern long long __lshrdi3(long long a, int b);

/* Zero is the width of the type: __clzdi2(0) is 64, __clzsi2(0) is 32. */
extern int __clzsi2(unsigned int x);
extern int __clzdi2(unsigned long long x);
extern int __ctzsi2(unsigned int x);
extern int __ctzdi2(unsigned long long x);
extern int __popcountsi2(unsigned int x);
extern int __popcountdi2(unsigned long long x);
extern int __paritysi2(unsigned int x);
extern int __paritydi2(unsigned long long x);
extern unsigned int __bswapsi2(unsigned int x);
extern unsigned long long __bswapdi2(unsigned long long x);

/* The low 64 bits of the product, which is all the ABI asks for. */
extern long long __muldi3(long long a, long long b);

/*
 * The 128-bit ABI.
 *
 * libgcc's TImode helpers take a 128-bit value as one object. The compiler
 * this library exists for parses the type but has no values of that width, so
 * the spelling here is the pair itself: lo is the low 64 bits, hi the high
 * ones. That is how the value lies in memory, and on x86-64 SysV it is how it
 * travels in a register pair as well, so the machine ABI is still libgcc's and
 * a call emitted by gcc or clang reaches these definitions. test/abi128.c
 * measures that with the compilers that can spell the type.
 *
 * The signed helpers take the same pair. Two's complement is the low word plus
 * the high word, so __divti3 and __udivti3 have the same argument type and
 * differ in what they make of it.
 */
typedef struct libvcc_u128 {
	unsigned long long lo;
	unsigned long long hi;
} libvcc_u128;

extern libvcc_u128 __udivmodti4(libvcc_u128 n, libvcc_u128 d,
                                libvcc_u128 *rem);
extern libvcc_u128 __udivti3(libvcc_u128 n, libvcc_u128 d);
extern libvcc_u128 __umodti3(libvcc_u128 n, libvcc_u128 d);
extern libvcc_u128 __divmodti4(libvcc_u128 a, libvcc_u128 b,
                               libvcc_u128 *rem);
extern libvcc_u128 __divti3(libvcc_u128 a, libvcc_u128 b);
extern libvcc_u128 __modti3(libvcc_u128 a, libvcc_u128 b);

/* The low 128 bits of the product, which is what the ABI asks for. */
extern libvcc_u128 __multi3(libvcc_u128 a, libvcc_u128 b);

/* Zero is the width of the type: __clzti2(0) is 128. */
extern int __clzti2(libvcc_u128 x);
extern int __ctzti2(libvcc_u128 x);
extern int __popcountti2(libvcc_u128 x);
extern int __parityti2(libvcc_u128 x);

/* Counts are 0 through 127. Anything else is outside the ABI and not
   tested; a count of 128 or more answers what libgcc answers, which is zero
   for the two logical shifts and the sign for the arithmetic one. */
extern libvcc_u128 __ashlti3(libvcc_u128 a, int b);
extern libvcc_u128 __ashrti3(libvcc_u128 a, int b);
extern libvcc_u128 __lshrti3(libvcc_u128 a, int b);

/*
 * The complex arithmetic helpers.
 *
 * A compiler emits one of these for `a * b` or `a / b` on a complex type it
 * has no instruction for. The arguments are the four components as scalars,
 * the dividend's real and imaginary parts first: __muldc3 (ar, ai, br, bi).
 * The value is handed back the way a complex value travels rather than as a
 * struct: xmm0 and xmm1 for double, xmm0 packed for float, and st0 for the
 * real part and st1 for the imaginary one when the component is a long
 * double. test/abi_complex.c declares these with the compiler's own complex
 * type and compares them with the operator.
 *
 * Division is defined here where the integer helpers leave it open, because
 * a complex quotient of zero is a value and not a wrong number: a nonzero
 * divided by zero is infinite, an infinity divided by a finite value is
 * infinite, and a finite value divided by an infinity is zero. NaN over NaN
 * is NaN.
 */
extern double _Complex __muldc3(double a, double b, double c, double d);
extern double _Complex __divdc3(double a, double b, double c, double d);
extern float _Complex __mulsc3(float a, float b, float c, float d);
extern float _Complex __divsc3(float a, float b, float c, float d);
extern long double _Complex __mulxc3(long double a, long double b, long double c,
                                     long double d);
extern long double _Complex __divxc3(long double a, long double b, long double c,
                                     long double d);

#endif /* LIBVCC_BUILTINS_H */
