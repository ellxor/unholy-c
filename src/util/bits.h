#ifndef BITS_H_
#define BITS_H_

// define builtins for non-gcc/clang compilers
#if !defined(__clang__) && !defined(__GNUC__)
#include <assert.h>

static inline
int __builtin_clz(unsigned int x) {
	assert(x != 0);
	int index = 0;

	for (int shift = 16; shift >= 1; shift >>= 1) {
		if (x >= 1 << shift) {
			index += shift;
			x >>= shift;
		}
	}

	return 31 - index;
}

#endif
#endif //BITS_H_
