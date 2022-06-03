#include "string_allocator.h"

#include "bits.h"
#include <stdlib.h>
#include <string.h>

enum {
	DEFAULT_CAPACITY = 1 << 16, //65kb
};

struct StringAllocator init_string_allocator() {
	struct StringAllocator allocator;

	allocator.capacity = DEFAULT_CAPACITY;
	allocator.mem = malloc(DEFAULT_CAPACITY);

	if (!allocator.mem) {
		// TODO: throw error
	}

	allocator.index = 0;
	return allocator;
}

void free_string_allocator(struct StringAllocator *allocator) {
	free(allocator->mem);
	allocator->capacity = 0;
	allocator->index = 0;
}

char *store_string(struct StringAllocator *allocator, const char *mem, int length) {
	if (allocator->capacity - allocator->index <= length) {
		// capacity = next power of 2 greater than minimum
		allocator->capacity = allocator->index + length + 1;
		allocator->capacity = 0x80000000 >> (__builtin_clz(allocator->capacity) - 1);

		allocator->mem = realloc(allocator->mem, allocator->capacity);

		if (!allocator->mem) {
			// TODO: throw error
		}
	}

	char *string = &allocator->mem[allocator->index];

	memcpy(string, mem, length);
	string[length] = '\0';

	allocator->index += length + 1;
	return string;
}
