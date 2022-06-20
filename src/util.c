#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// murmur2 hash function
unsigned hash(const char *in, int length) {
	unsigned digest = 0;
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


// simple vectors
enum {
	DEFAULT_VEC_LENGTH = 1024,
};

struct Vec init_vector(int elem_size) {
	struct Vec vec;

	vec.capacity = DEFAULT_VEC_LENGTH * elem_size;
	vec.elem_size = elem_size;
	vec.length = 0;
	vec.used = 0;

	vec.mem = malloc(vec.capacity);
	if (!vec.mem) errx("out of memory: failed to allocate %d bytes", vec.capacity);

	return vec;
}

void vec_push(struct Vec *vec, void *elem) {
	if (vec->used >= vec->capacity) {
		vec->capacity <<= 1;
		vec->mem = realloc(vec->mem, vec->capacity);

		if (!vec->mem) {
			errx("out of memory: failed to allocate %d bytes", vec->capacity);
		}
	}

	memcpy(vec->mem + vec->used, elem, vec->elem_size);
	vec->used += vec->elem_size;
	vec->length++;
}

void vec_free(struct Vec *vec) {
	free(vec->mem);
	vec->capacity = 0;
	vec->elem_size = 0;
	vec->length = 0;
	vec->used = 0;
}

// compiler errors
const char *program = NULL;

void set_program(const char *prog) {
	program = prog;
}

void errx(const char *fmt, ...) {
	printf(WHITE "%s: " RED "error: " RESET, program);

	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);
	va_end(args);

	putchar('\n');
	exit(1);
}
