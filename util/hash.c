#include "hash.h"

// murmur2 hash function
unsigned hash(const char *in, int length) {
	unsigned digest = length;
	enum { shuffle = 0x5bd1e995 };

	for (int i = 0; i < length >> 2; i++) {
		unsigned block = *(unsigned *)in;

		block *= shuffle;
		block ^= block >> 24;
		block *= shuffle;

		digest *= shuffle;
		digest ^= block;

		in += 4;
	}

	switch (length & 3) {
		case 3: digest ^= in[2] << 16;
		case 2: digest ^= in[1] << 8;
		case 1: digest ^= in[0];
		        digest *= shuffle;
	}

	digest ^= digest >> 13;
	digest *= shuffle;
	digest ^= digest >> 15;

	return digest;
}
