#ifndef LEXER_H_
#define LEXER_H_

struct Lexer {
	const char *filename;
	const char *stream, *reference;
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

#endif //LEXER_H_