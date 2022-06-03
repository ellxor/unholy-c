#ifndef STRING_ALLOC_H_
#define STRING_ALLOC_H_

struct StringAllocator {
	char *mem;
	int index;
	int capacity;
};

struct StringAllocator init_string_allocator();
char *store_string(struct StringAllocator *, const char *, int);
void free_string_allocator(struct StringAllocator *);

#endif //STRING_ALLOC_H_