#ifndef ALLOC_H_
#define ALLOC_H_

struct Allocator {
	char *mem;
	int index;
	int capacity;
};

struct Allocator init_allocator();
char *store_string(struct Allocator *, const char *, int);
void *store_object(struct Allocator *, const void *, int);
void free_allocator(struct Allocator *);

#endif //ALLOC_H_
