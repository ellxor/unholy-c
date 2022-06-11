#ifndef LEXER_H_
#define LEXER_H_

#include <stdbool.h>

#include "tokens.h"
#include "util/allocator.h"
#include "util/vec.h"
#include "util/util.h"

struct Lexer {
	const char *filename;
	const char *stream, *start;
	int line, col;
	bool errors;

	struct Allocator *allocator;
};

void lex_line(struct Lexer *, struct Vec *tokens);
void lex_file(const char *filename, struct Allocator *, struct Vec *tokens);

// lexer_keyword.c
enum TokenType lookup_keyword(const char *, int, enum TokenType);

// lexer errors

enum LexerErrorType {
	NOTE, WARNING, ERROR,
};

// offset is location of error, if NULL, then lexer->col is used instead
void lexer_err(struct Lexer *lexer, enum LexerErrorType, const char *offset, const char *fmt, ...) PRINTF(4,5);

#endif //LEXER_H_
