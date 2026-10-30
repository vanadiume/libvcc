#include <libvcc/builtins.h>
#include <libvcc/compiler.h>

/*
 * The counts answer the width for zero. gcc's __builtin_clzll leaves zero
 * undefined, so the guard is not an optimization; a helper that was handed
 * zero has to answer something.
 */

int __clzsi2(unsigned int x)
{
	if (x == 0)
		return 32;
#if LIBVCC_HAVE_BUILTIN_CLZLL
	return __builtin_clzll((unsigned long long)x) - 32;
#else
	{
		int n;

		n = 0;
		while ((x & 0x80000000U) == 0) {
			x <<= 1;
			n++;
		}
		return n;
	}
#endif
}

int __clzdi2(unsigned long long x)
{
	if (x == 0)
		return 64;
#if LIBVCC_HAVE_BUILTIN_CLZLL
	return __builtin_clzll(x);
#else
	{
		int n;

		n = 0;
		while ((x & 0x8000000000000000ULL) == 0) {
			x <<= 1;
			n++;
		}
		return n;
	}
#endif
}

int __ctzsi2(unsigned int x)
{
	int n;

	if (x == 0)
		return 32;
	n = 0;
	while ((x & 1U) == 0) {
		x >>= 1;
		n++;
	}
	return n;
}

int __ctzdi2(unsigned long long x)
{
	int n;

	if (x == 0)
		return 64;
	n = 0;
	while ((x & 1ULL) == 0) {
		x >>= 1;
		n++;
	}
	return n;
}

int __popcountsi2(unsigned int x)
{
	int n;
	unsigned int y;

	n = 0;
	while (x != 0) {
		y = x - 1U;
		x &= y;
		n++;
	}
	return n;
}

int __popcountdi2(unsigned long long x)
{
	int n;
	unsigned long long y;

	n = 0;
	while (x != 0) {
		y = x - 1ULL;
		x &= y;
		n++;
	}
	return n;
}

int __paritysi2(unsigned int x)
{
	int p;
	unsigned int y;

	p = 0;
	while (x != 0) {
		p ^= 1;
		y = x - 1U;
		x &= y;
	}
	return p;
}

int __paritydi2(unsigned long long x)
{
	int p;
	unsigned long long y;

	p = 0;
	while (x != 0) {
		p ^= 1;
		y = x - 1ULL;
		x &= y;
	}
	return p;
}

unsigned int __bswapsi2(unsigned int x)
{
	return ((x & 0x000000ffU) << 24) | ((x & 0x0000ff00U) << 8) |
	       ((x & 0x00ff0000U) >> 8) | ((x & 0xff000000U) >> 24);
}

unsigned long long __bswapdi2(unsigned long long x)
{
	return ((x & 0x00000000000000ffULL) << 56) |
	       ((x & 0x000000000000ff00ULL) << 40) |
	       ((x & 0x0000000000ff0000ULL) << 24) |
	       ((x & 0x00000000ff000000ULL) << 8) |
	       ((x & 0x000000ff00000000ULL) >> 8) |
	       ((x & 0x0000ff0000000000ULL) >> 24) |
	       ((x & 0x00ff000000000000ULL) >> 40) |
	       ((x & 0xff00000000000000ULL) >> 56);
}
