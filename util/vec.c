#include "vec.h"
#include "error.h"

#include <stdlib.h>
#include <string.h>

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
	if (!vec.mem) internal_error();

	return vec;
}

void vec_push(struct Vec *vec, void *elem) {
	if (vec->used >= vec->capacity) {
		vec->capacity <<= 1;
		vec->mem = realloc(vec->mem, vec->capacity);

		if (!vec->mem) {
			internal_error();
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