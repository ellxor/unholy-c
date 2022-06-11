#include "allocator.h"

#include "bits.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>

enum {
	DEFAULT_CAPACITY = 1 << 14, //16kb
};

struct Allocator init_allocator() {
	struct Allocator allocator;

	allocator.capacity = DEFAULT_CAPACITY;
	allocator.mem = malloc(DEFAULT_CAPACITY);

	if (!allocator.mem)
		errx("out of memory: failed to allocate %d bytes", allocator.capacity);

	allocator.index = 0;
	return allocator;
}

void free_allocator(struct Allocator *allocator) {
	free(allocator->mem);
	allocator->capacity = 0;
	allocator->index = 0;
}

void
expand_allocator(struct Allocator *allocator, int size) {
	// capacity = next power of 2 greater than minimum
	allocator->capacity = allocator->index + size;
	allocator->capacity = 0x80000000 >> (__builtin_clz(allocator->capacity) - 1);
	allocator->mem = realloc(allocator->mem, allocator->capacity);

	if (!allocator->mem) {
		errx("out of memory: failed to allocate %d bytes", allocator->capacity);
	}
}

char *store_string(struct Allocator *allocator, const char *mem, int length) {
	if (allocator->capacity - allocator->index <= length)
		expand_allocator(allocator, length + 1);

	char *string = &allocator->mem[allocator->index];
	memcpy(string, mem, length);
	string[length] = '\0';

	allocator->index += length + 1;
	return string;
}


void *store_object(struct Allocator *allocator, const void *mem, int size) {
	if  (allocator->capacity - allocator->index < size)
		expand_allocator(allocator, size);

	void *dst = &allocator->mem[allocator->index];
	memcpy(dst, mem, size);

	allocator->index += size;
	return dst;
}
