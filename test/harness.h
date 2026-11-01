#ifndef LIBVCC_TEST_HARNESS_H
#define LIBVCC_TEST_HARNESS_H

/*
 * A check counts itself and prints the failure at the line it was written on.
 * Nothing here needs a C99 format string or a variable-argument function: the
 * tests are compiled by the same three compilers as the library, and a test
 * harness that only works under gcc would hide the cases that matter.
 */

#define LIBVCC_LL_MIN (-9223372036854775807LL - 1)
#define LIBVCC_LL_MAX 9223372036854775807LL
#define LIBVCC_INT_MIN (-2147483647 - 1)
#define LIBVCC_INT_MAX 2147483647

extern unsigned long long libvcc_test_checks;
extern unsigned long long libvcc_test_failures;

void libvcc_test_check_u64(const char *file, int line, const char *what,
                           unsigned long long got, unsigned long long want);
void libvcc_test_check_ll(const char *file, int line, const char *what,
                          long long got, long long want);
void libvcc_test_check_true(const char *file, int line, const char *what,
                            int ok);

/* Decimal, to the test binary's stdout. The 128-bit checker prints its own
   line because a format string wide enough for two words is not something the
   matrix's compilers all take. */
void libvcc_test_put_u64(unsigned long long v);

/* xorshift64. The state must not be zero. */
unsigned long long libvcc_test_random(unsigned long long *state);

int libvcc_test_report(const char *name, unsigned long long checks,
                       unsigned long long failures);
int libvcc_test_total(void);

#define CHECK_U64(got, want) \
	libvcc_test_check_u64(__FILE__, __LINE__, #got " == " #want, \
	                      (unsigned long long)(got), \
	                      (unsigned long long)(want))

#define CHECK_LL(got, want) \
	libvcc_test_check_ll(__FILE__, __LINE__, #got " == " #want, \
	                     (long long)(got), (long long)(want))

#define CHECK(cond) \
	libvcc_test_check_true(__FILE__, __LINE__, #cond, (cond) ? 1 : 0)

#endif /* LIBVCC_TEST_HARNESS_H */
