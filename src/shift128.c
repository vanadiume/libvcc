#include <libvcc/builtins.h>

/*
 * The 128-bit shifts, one 64-bit word at a time.
 *
 * On a 64-bit machine gcc and clang fold each of these back into a shift pair
 * and never call the file; on a 32-bit machine a 128-bit shift is a call and
 * this is what it can reach. So none of the three may be written with a
 * 128-bit shift.
 *
 * The count is the ABI's: 0 through 127. A larger count is answered the way
 * libgcc answers it rather than shifted, because a shift by the width or more
 * is undefined and a helper that leaves it undefined is a helper whose answer
 * depends on the compiler that built it.
 *
 * Each half is computed into a named temporary before the two are combined,
 * the same way src/shift.c does it, for the same reason: vcc refuses an
 * arithmetic value used directly as an operand of a shift.
 */

libvcc_u128 __ashlti3(libvcc_u128 a, int b)
{
	libvcc_u128 r;

	if (b <= 0)
		return a;
	if (b >= 128) {
		r.lo = 0;
		r.hi = 0;
		return r;
	}
	if (b >= 64) {
		r.hi = a.lo << (b - 64);
		r.lo = 0;
	} else {
		r.hi = (a.hi << b) | (a.lo >> (64 - b));
		r.lo = a.lo << b;
	}
	return r;
}

libvcc_u128 __lshrti3(libvcc_u128 a, int b)
{
	libvcc_u128 r;

	if (b <= 0)
		return a;
	if (b >= 128) {
		r.lo = 0;
		r.hi = 0;
		return r;
	}
	if (b >= 64) {
		r.lo = a.hi >> (b - 64);
		r.hi = 0;
	} else {
		r.lo = (a.lo >> b) | (a.hi << (64 - b));
		r.hi = a.hi >> b;
	}
	return r;
}

/*
 * The fill is written out instead of shifting a sign bit into place, because a
 * right shift of a value with the top bit set is implementation-defined in
 * C89. The sign is the top bit of hi, wherever the count lands.
 */

libvcc_u128 __ashrti3(libvcc_u128 a, int b)
{
	libvcc_u128 r;
	unsigned long long fill, l, h;

	fill = (a.hi & 0x8000000000000000ULL) ? 0xffffffffffffffffULL : 0ULL;
	if (b <= 0)
		return a;
	if (b >= 128) {
		r.lo = fill;
		r.hi = fill;
		return r;
	}
	if (b >= 64) {
		r.lo = b == 64 ? a.hi : (a.hi >> (b - 64)) | (fill << (128 - b));
		r.hi = fill;
	} else {
		l = a.hi << (64 - b);
		r.lo = (a.lo >> b) | l;
		h = fill << (64 - b);
		r.hi = (a.hi >> b) | h;
	}
	return r;
}
