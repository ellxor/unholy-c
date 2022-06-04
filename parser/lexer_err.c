#include "lexer.h"

#include <stdarg.h>
#include <stdio.h>

void lexer_err(struct Lexer *lexer, enum LexerErrorType type, const char *offset, const char *fmt, ...) {
	// print location info
	int column = offset ? lexer->col + (offset - lexer->stream) : lexer->col;
	printf(WHITE"%s:%d:%d: ", lexer->filename, lexer->line, column);

	// print coloured error type
	switch (type) {
		case NOTE:	printf(GREY "note: " RESET);
				break;

		case WARNING:	printf(MAGENTA "warning: " RESET);
				break;

		case ERROR:	printf(RED "error: " RESET);
				break;
	}

	// print message
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	// print context
	printf("\n%5d | %s\n", lexer->line, lexer->start);
	printf("      | ");

	int indent = (offset ?: lexer->stream) - lexer->start;
	int length = offset ? (lexer->stream - offset - 1) : 0;

	for (int i = 0; i < indent; i++) putchar(' ');
	putchar('^');

	for (int i = 0; i < length; i++) putchar('~');
	putchar('\n');
	putchar('\n');

	if (type == ERROR)
		lexer->errors = true;
}