#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>
#include "util/util.h"

struct Lexer {
	const char *filename;
	const char *stream, *start;
	int line, col;
	int errors;
};

static inline
char peek_next(struct Lexer *lexer) {
	return *lexer->stream;
}

static inline
char chop_next(struct Lexer *lexer) {
	return lexer->col++, *lexer->stream++;
}

int chop_int(struct Lexer *lexer);
int chop_string(struct Lexer *lexer, char *buffer);
int chop_identifier(struct Lexer *lexer, char *buffer);

enum LexerErrorType {
	NOTE, WARNING, ERROR,
};

void lexer_err(struct Lexer *lexer, enum LexerErrorType, const char *offset, const char *fmt, ...) PRINTF(4,5);

#endif //LEXER_H_