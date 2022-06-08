#ifndef LEXER_H_
#define LEXER_H_

#include <stdbool.h>

#include "tokens.h"
#include "../util/allocator.h"
#include "../util/vec.h"
#include "../util/util.h"

struct Lexer {
	const char *filename;
	const char *stream, *start;
	int line, col;
	bool errors;

	struct Allocator *allocator;
};

static inline
char peek_next(struct Lexer *lexer) {
	return *lexer->stream;
}

static inline
char chop_next(struct Lexer *lexer) {
	return lexer->col++, *lexer->stream++;
}

// lexer_simple.c
int chop_int(struct Lexer *lexer);
int chop_string(struct Lexer *lexer, char *buffer);
int chop_identifier(struct Lexer *lexer, char *buffer);

// lexer_keyword.c
enum TokenType lookup_keyword(const char *, int, enum TokenType);

// lexer.c
void lex_line(struct Lexer *, struct Vec *tokens);
void lex_file(const char *filename, struct Allocator *, struct Vec *tokens);

// lexer_err.c
enum LexerErrorType {
	NOTE, WARNING, ERROR,
};

void lexer_err(struct Lexer *lexer, enum LexerErrorType, const char *offset, const char *fmt, ...) PRINTF(4,5);

#endif //LEXER_H_
