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
 * Division by zero is not defined by the ABI. __udivmoddi4 and
 * __udivmodsi4 answer zero with the remainder set to the dividend, so a
 * caller that divides by zero gets a wrong number and not a trap. A compiler
 * is expected to have diagnosed the constant case before it emitted a call.
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

#endif /* LIBVCC_BUILTINS_H */
