#ifndef LEXER_H_
#define LEXER_H_

#include <stdbool.h>

#include "tokens.h"
#include "util/allocator.h"
#include "util/vec.h"

struct Lexer {
	const char *filename;
	const char *stream, *start;
	int line, col;
	int errors;

	struct Allocator *allocator;
};

void lex_line(struct Lexer *, struct Vec *tokens);
void lex_file(const char *filename, struct Allocator *, struct Vec *tokens);

#endif //LEXER_H_
