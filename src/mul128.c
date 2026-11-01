#include <libvcc/builtins.h>

/*
 * The low 128 bits of the product of two 128-bit values, from 16-bit limbs.
 *
 * The product is a convolution: limb k of the answer is the sum of a[i] * b[j]
 * over every pair that adds up to k. Each such product is at most (2^16 - 1)^2
 * and there are at most eight of them to a limb, so a 64-bit accumulator holds
 * the sum and the carry out of each limb is the next limb's input.
 *
 * The limbs are 16 bits because that is the largest size whose product fits in
 * the 32-bit type the language promises, which means no multiply in this file
 * is wider than the machine is required to have. A 64-bit limb pair would be
 * three multiplies instead of thirty-six, and would also be a call to
 * __muldi3 on the target that has no 64-bit multiply, which is the target this
 * helper is for.
 *
 * The high half of a full product is not part of this ABI: the helper answers
 * what the caller keeps, which is the low 128 bits.
 */

static void wide_limbs(libvcc_u128 v, unsigned int limb[8])
{
	limb[0] = (unsigned int)(v.lo & 0xffffULL);
	limb[1] = (unsigned int)((v.lo >> 16) & 0xffffULL);
	limb[2] = (unsigned int)((v.lo >> 32) & 0xffffULL);
	limb[3] = (unsigned int)((v.lo >> 48) & 0xffffULL);
	limb[4] = (unsigned int)(v.hi & 0xffffULL);
	limb[5] = (unsigned int)((v.hi >> 16) & 0xffffULL);
	limb[6] = (unsigned int)((v.hi >> 32) & 0xffffULL);
	limb[7] = (unsigned int)((v.hi >> 48) & 0xffffULL);
}

libvcc_u128 __multi3(libvcc_u128 a, libvcc_u128 b)
{
	unsigned int al[8], bl[8], c[8];
	unsigned long long acc[8];
	libvcc_u128 r;
	int i, j, k;

	wide_limbs(a, al);
	wide_limbs(b, bl);
	for (k = 0; k < 8; k++)
		acc[k] = 0ULL;
	for (i = 0; i < 8; i++)
		for (j = 0; j + i < 8; j++)
			acc[i + j] += (unsigned long long)(al[i] * bl[j]);
	for (k = 0; k < 7; k++) {
		c[k] = (unsigned int)(acc[k] & 0xffffULL);
		acc[k + 1] += acc[k] >> 16;
	}
	c[7] = (unsigned int)(acc[7] & 0xffffULL);
	r.lo = (unsigned long long)c[0] | ((unsigned long long)c[1] << 16) |
	       ((unsigned long long)c[2] << 32) |
	       ((unsigned long long)c[3] << 48);
	r.hi = (unsigned long long)c[4] | ((unsigned long long)c[5] << 16) |
	       ((unsigned long long)c[6] << 32) |
	       ((unsigned long long)c[7] << 48);
	return r;
}
