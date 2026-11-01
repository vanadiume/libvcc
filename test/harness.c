#include <stdio.h>

#include "harness.h"

unsigned long long libvcc_test_checks;
unsigned long long libvcc_test_failures;

static void put_u64(unsigned long long v)
{
	char digits[20];
	int n;

	n = 0;
	do {
		digits[n] = (char)('0' + (int)(v % 10));
		n++;
		v /= 10;
	} while (v != 0);
	while (n > 0) {
		n--;
		fputc(digits[n], stdout);
	}
}

void libvcc_test_put_u64(unsigned long long v)
{
	put_u64(v);
}

static void put_ll(long long v)
{
	if (v < 0) {
		fputc('-', stdout);
		put_u64(0 - (unsigned long long)v);
	} else {
		put_u64((unsigned long long)v);
	}
}

static void put_prefix(const char *file, int line, const char *what)
{
	fputs("FAIL ", stdout);
	fputs(file, stdout);
	fputc(':', stdout);
	put_u64((unsigned long long)line);
	fputs(": ", stdout);
	fputs(what, stdout);
}

void libvcc_test_check_u64(const char *file, int line, const char *what,
                           unsigned long long got, unsigned long long want)
{
	libvcc_test_checks++;
	if (got == want)
		return;
	libvcc_test_failures++;
	put_prefix(file, line, what);
	fputs(": got ", stdout);
	put_u64(got);
	fputs(" want ", stdout);
	put_u64(want);
	fputc('\n', stdout);
}

void libvcc_test_check_ll(const char *file, int line, const char *what,
                          long long got, long long want)
{
	libvcc_test_checks++;
	if (got == want)
		return;
	libvcc_test_failures++;
	put_prefix(file, line, what);
	fputs(": got ", stdout);
	put_ll(got);
	fputs(" want ", stdout);
	put_ll(want);
	fputc('\n', stdout);
}

void libvcc_test_check_true(const char *file, int line, const char *what,
                            int ok)
{
	libvcc_test_checks++;
	if (ok)
		return;
	libvcc_test_failures++;
	put_prefix(file, line, what);
	fputs(": false\n", stdout);
}

unsigned long long libvcc_test_random(unsigned long long *state)
{
	unsigned long long x;

	x = *state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	*state = x;
	return x;
}

int libvcc_test_report(const char *name, unsigned long long checks,
                       unsigned long long failures)
{
	fputs(name, stdout);
	fputs(": ", stdout);
	if (failures != 0)
		fputs("FAILED, ", stdout);
	put_u64(checks);
	fputs(" checks, ", stdout);
	put_u64(failures);
	fputs(" failures\n", stdout);
	return failures == 0;
}

int libvcc_test_total(void)
{
	fputs("total: ", stdout);
	put_u64(libvcc_test_checks);
	fputs(" checks, ", stdout);
	put_u64(libvcc_test_failures);
	fputs(" failures\n", stdout);
	return libvcc_test_failures == 0;
}
