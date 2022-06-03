#ifndef VEC_H_
#define VEC_H_

struct Vec {
	void *mem;
	int length, used, capacity;
	int elem_size;
};

#define vec(T) init_vector(sizeof(T))

struct Vec init_vector(int elem_size);
void vec_push(struct Vec *, void *elem);
void vec_free(struct Vec *);

#endif //VEC_H_