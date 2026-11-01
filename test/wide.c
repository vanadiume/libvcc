#include <stdio.h>

#include "harness.h"
#include "wide.h"

libvcc_u128 libvcc_test_wide(unsigned long long lo, unsigned long long hi)
{
	libvcc_u128 v;

	v.lo = lo;
	v.hi = hi;
	return v;
}

void libvcc_test_check_wide(const char *file, int line, const char *what,
                            libvcc_u128 got, libvcc_u128 want)
{
	libvcc_test_checks++;
	if (got.lo == want.lo && got.hi == want.hi)
		return;
	libvcc_test_failures++;
	fputs("FAIL ", stdout);
	fputs(file, stdout);
	fputc(':', stdout);
	libvcc_test_put_u64((unsigned long long)line);
	fputs(": ", stdout);
	fputs(what, stdout);
	fputs(": got hi:", stdout);
	libvcc_test_put_u64(got.hi);
	fputs(" lo:", stdout);
	libvcc_test_put_u64(got.lo);
	fputs(" want hi:", stdout);
	libvcc_test_put_u64(want.hi);
	fputs(" lo:", stdout);
	libvcc_test_put_u64(want.lo);
	fputc('\n', stdout);
}
