#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stdnoreturn.h>

// PRINTF type checking
#if defined(__clang__) || defined(__GNUC__)
#define PRINTF(fmt, first) __attribute__((format (printf, fmt, first)))
#else
#define PRINTF(fmt, first)
#endif

// ANSI CODES (8-bit terminal colors)
#define RED      "\e[31;1m"
#define MAGENTA  "\e[35;1m"
#define GREY     "\e[90;1m"
#define WHITE    "\e[97;1m"
#define RESET    "\e[0m"

// BIT INTRINSICS
// define count-leading-zeros intrinsic for other compilers
#if !defined(__clang__) && !defined(__GNUC__)

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

static inline
int max(int a, int b) { return (a > b) ? a : b; }

static inline
int min(int a, int b) { return (a < b) ? a : b; }


// string hashing
unsigned hash(const char *, int length);


// simple vector implementation
struct Vec {
	void *mem;
	int length, used, capacity;
	int elem_size;
};

#define vec(T) init_vector(sizeof(T))

struct Vec init_vector(int elem_size);
void vec_push(struct Vec *, void *elem);
void vec_free(struct Vec *);


// compiler errors
void set_program(const char *prog);
noreturn void errx(const char *fmt, ...) PRINTF(1,2);

#endif //UTIL_H
