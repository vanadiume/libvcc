#include <stdio.h>

#include "harness.h"
#include "tests.h"

struct libvcc_test_group {
	const char *name;
	void (*run)(void);
};

int main(void)
{
	static const struct libvcc_test_group groups[] = {
		{ "div64", libvcc_test_div64 },
		{ "div32", libvcc_test_div32 },
		{ "div128", libvcc_test_div128 },
		{ "shift", libvcc_test_shift },
		{ "shift128", libvcc_test_shift128 },
		{ "bits", libvcc_test_bits },
		{ "bits128", libvcc_test_bits128 },
		{ "mul", libvcc_test_mul },
		{ "mul128", libvcc_test_mul128 },
		{ "abi128", libvcc_test_abi128 }
	};
	unsigned long i;
	unsigned long long checks, failures;
	int failed;

	failed = 0;
	for (i = 0; i < sizeof(groups) / sizeof(groups[0]); i++) {
		checks = libvcc_test_checks;
		failures = libvcc_test_failures;
		groups[i].run();
		if (!libvcc_test_report(groups[i].name,
		                        libvcc_test_checks - checks,
		                        libvcc_test_failures - failures))
			failed = 1;
	}
	if (!libvcc_test_total())
		failed = 1;
	return failed;
}
