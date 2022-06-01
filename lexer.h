#ifndef LEXER_H_
#define LEXER_H_

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

#endif //LEXER_H_