#ifndef STRING_ALLOC_H_
#define STRING_ALLOC_H_

struct Allocator {
	char *mem;
	int index;
	int capacity;
};

struct Allocator init_allocator();
char *store_string(struct Allocator *, const char *, int);
void free_allocator(struct Allocator *);

#endif //STRING_ALLOC_H_
