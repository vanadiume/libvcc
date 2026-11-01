#ifndef LIBVCC_TEST_WIDE_H
#define LIBVCC_TEST_WIDE_H

/*
 * The 128-bit helpers are checked a whole value at a time, so the harness
 * needs the ABI's pair. This lives beside harness.h rather than in it because
 * test/abi128.c declares the same symbols with the compiler's own __int128 and
 * the two declarations cannot share a translation unit.
 */

#include <libvcc/builtins.h>

#define WIDE(lo, hi) libvcc_test_wide((unsigned long long)(lo), \
                                      (unsigned long long)(hi))

#define CHECK_WIDE(got, want) \
	libvcc_test_check_wide(__FILE__, __LINE__, #got " == " #want, \
	                       (got), (want))

libvcc_u128 libvcc_test_wide(unsigned long long lo, unsigned long long hi);

void libvcc_test_check_wide(const char *file, int line, const char *what,
                            libvcc_u128 got, libvcc_u128 want);

#endif /* LIBVCC_TEST_WIDE_H */
