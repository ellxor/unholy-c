#include "lexer.h"

#include "keywords.h"
#include "tokens.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/allocator.h"
#include "util/error.h"
#include "util/hash.h"
#include "util/vec.h"

enum {
	MAX_BUFFER_SIZE = 1024,
	MAX_LINE_LENGTH = 120,
};

enum LexerErrorType {
	NOTE, WARNING, ERROR,
};

// offset is location of error, if NULL, then lexer->col is used instead
void lexer_err(struct Lexer *lexer, enum LexerErrorType, const char *offset, const char *fmt, ...) PRINTF(4,5);

static
const char multichar[][2] = {
	"<<", ">>", "==", "!=", "<=", ">=", "&&", "||", "++", "--", "::",
};


static inline
char peek_next(struct Lexer *lexer) {
	return *lexer->stream;
}

static inline
char chop_next(struct Lexer *lexer) {
	return lexer->col++, *lexer->stream++;
}


static inline
int chop_digit(struct Lexer *lexer) {
	char c = peek_next(lexer);

	if ('0' <= c && c <= '9')
		return chop_next(lexer) - '0';

	lexer_err(lexer, ERROR, NULL, "expected digit, found `%c`", c);
	return chop_next(lexer);
}

static inline
int chop_hex_digit(struct Lexer *lexer) {
	char c = peek_next(lexer);

	if ('0' <= c && c <= '9')       return chop_next(lexer) - '0';
	else if ('a' <= c && c <= 'f')  return chop_next(lexer) - 'a';
	else if ('A' <= c && c <= 'F')  return chop_next(lexer) - 'A';

	lexer_err(lexer, ERROR, NULL, "expected hex digit, found `%c`", c);
	return chop_next(lexer);
}


static
int chop_int(struct Lexer *lexer) {
	int result = 0, digits = 0, base = 10;
	bool overflow = false;

	// keep copy of start pointer
	const char *start = lexer->stream;

	// select base
	if (peek_next(lexer) == '0') {
		chop_next(lexer);
		digits++;

		switch (peek_next(lexer)) {
			case 'b': base = 2;  break;
			case 'B': base = 2;  break;
			case 'x': base = 16; break;
			case 'X': base = 16; break;
		}

		if (base != 10) {
			chop_next(lexer);
			digits = 0;
		}
	}

	enum { DIGIT_SEPERATOR = '\'' };

	while (isalnum(peek_next(lexer)) || peek_next(lexer) == DIGIT_SEPERATOR) {
		if (peek_next(lexer) == DIGIT_SEPERATOR) {
			chop_next(lexer);
			continue;
		}

		char digit = (base == 16) ? chop_hex_digit(lexer) : chop_digit(lexer);
		digits++;

		if (base == 2 && digit >= 2) {
			lexer_err(lexer, ERROR, lexer->stream - 1, "binary digit `%c` is not 0 or 1", '0' + digit);
		}

		result *= base;
		result += digit;

		if (result < 0)
			overflow = true;
	}

	if (digits == 0) {
		lexer_err(lexer, ERROR, NULL, "expected digits after base specifier");
	}

	if (overflow) {
		int length = lexer->stream - start;
		lexer_err(lexer, ERROR, start, "integer constant overflows int type: `%.*s`", length, start);
	}

	return result;
}

static
int chop_string(struct Lexer *lexer, char quote, char *buffer) {
	// keep copy of base string
	const char *start = lexer->stream;
	int length = 0;

	// strip leading quote
	assert(peek_next(lexer) == quote);
	chop_next(lexer);

	while (peek_next(lexer) != quote) {
		char c = chop_next(lexer);

		if (c == '\0') {
			lexer_err(lexer, ERROR, start, "unterminated string constant");
			return -1;
		}

		// escape sequences
		if (c == '\\') {
			char escape = chop_next(lexer);

			switch (escape) {
				case 'e':  c = '\e'; break;
				case 'f':  c = '\f'; break;
				case 'n':  c = '\n'; break;
				case 'r':  c = '\r'; break;
				case 't':  c = '\t'; break;
				case 'v':  c = '\v'; break;
				case '"':  c = '\"'; break;
				case '\'': c = '\''; break;
				case '\\': break;

				// TODO: implement hex/unicode code points
				case 'x': lexer_err(lexer, ERROR, NULL, "hex code points are not implemented yet!"); break;
				case 'u': lexer_err(lexer, ERROR, NULL, "unicode code points are not implemented yet!"); break;
				case 'U': lexer_err(lexer, ERROR, NULL, "unicode code points are not implemented yet!"); break;

				case 0:
					lexer_err(lexer, ERROR, start, "unterminated string constant");
					return -1;

				default:
					lexer_err(lexer, WARNING, lexer->stream - 1, "invalid escape sequence `%c`", escape);
					lexer_err(lexer, NOTE, lexer->stream - 2, "`\\` character will be ignored");
					c = escape;
			}
		}

		*buffer++ = c;
		length++;
	}

	// strip trailing quote
	assert(peek_next(lexer) == quote);
	chop_next(lexer);

	return length;
}

static
int chop_identifier(struct Lexer *lexer, char *buffer) {
	int length = 0;

	while (isalnum(peek_next(lexer)) || peek_next(lexer) == '_') {
		char c = chop_next(lexer);
		*buffer++ = c;
		length++;
	}

	return length;
}


static
void chop_token(struct Lexer *lexer, struct Vec *tokens) {
	struct Token token = {
		.filename = lexer->filename,
		.line = lexer->line,
		.col  = lexer->col,
	};

	char buffer[MAX_BUFFER_SIZE];
	int length;

	switch (peek_next(lexer)) {
		case '_':
		case 'a' ... 'z':
		case 'A' ... 'Z':
			length = chop_identifier(lexer, buffer);
			enum TokenType type = lookup_keyword(buffer, length, KEYWORD);

			if (type == NONE) {
				token.type = IDENT;
				token.length = length;
				token.text = store_string(lexer->allocator, buffer, length);
			} else {
				token.type = type;
			}

			break;

		case '#':
			chop_next(lexer);

			length = chop_identifier(lexer, buffer);
			token.type = lookup_keyword(buffer, length, PREPROC);

			if (token.type == NONE) {
				lexer_err(lexer, ERROR, lexer->stream - length, "invalid preprocessor directive");
			}

			break;

		case '0' ... '9':
			token.type = INT;
			token.value = chop_int(lexer);
			break;

		case '"':
			token.type = STRING;

			length = chop_string(lexer, '"', buffer);
			if (length < 0) return; //string error

			token.text = store_string(lexer->allocator, buffer, length);
			break;

		case '\'':
			token.type = INT;

			length = chop_string(lexer, '\'', buffer);
			if (length < 0) return; //string error

			if (length > 1) {
				lexer_err(lexer, ERROR, lexer->stream - length - 2, "character constant is more than 1 character");
			}

			token.value = *buffer;
			break;

		case 0 ... 0x20:
		case 127: // DEL
		case '$':
		case '@':
		case '`':
		case '\\':
			if (isprint(peek_next(lexer))) {
				lexer_err(lexer, ERROR, NULL, "invalid ascii char `%c` in source file", peek_next(lexer));
			} else {
				lexer_err(lexer, ERROR, NULL, "invalid ascii char (%d) in source file", peek_next(lexer));
			}

			chop_next(lexer);
			break;

		default:
			token.type = PUNCTUATION;

			// handle multi-char punctuation
			int size = sizeof multichar / sizeof multichar[0];

			int a = chop_next(lexer);
			int b = peek_next(lexer);

			// store single character temporarily
			token.value = a;

			for (int i = 0; i < size; i++) {
				if (multichar[i][0] == a && multichar[i][1] == b) {
					token.value = multichar_mix(a,b);
					chop_next(lexer);
					break;
				}
			}

			break;
	}

	vec_push(tokens, &token);
}


void lex_line(struct Lexer *lexer, struct Vec *tokens) {
	while (peek_next(lexer) != '\0') {
		// skip whitespace
		if  (isspace(peek_next(lexer))) {
			chop_next(lexer);
			continue;
		}

		// remove comment to end of line
		struct Lexer tmp = *lexer;

		if (chop_next(&tmp) == '/' &&
		    chop_next(&tmp) == '/') {
			return;
		}

		chop_token(lexer, tokens);
	}
}


void lex_file(const char *filename, struct Allocator *allocator, struct Vec *tokens) {
	// open file for reading
	FILE *file = fopen(filename, "r");

	if (!file) {
		errx("file `%s` not found", filename);
	}

	// initialise lexer
	char buffer[MAX_BUFFER_SIZE];

	struct Lexer lexer = {
		.filename = filename,
		.start = buffer,
		.line = 1, .col = 1,
		.errors = false,
		.allocator = allocator,
	};

	// iterate lines
	while (fgets(buffer, sizeof buffer, file)) {
		int length = strcspn(buffer, "\n");
		buffer[length] = 0;

		if (length == MAX_BUFFER_SIZE - 1) {
			lexer_err(&lexer, ERROR, NULL, "line is too long");
		}

		else if (length > MAX_LINE_LENGTH) {
			lexer_err(&lexer, WARNING, NULL, "line exceeds %d chars", MAX_LINE_LENGTH);
		}

		// replace tabs with spaces to align error context
		//for (int i = 0; i < length; i++)
		//	if (buffer[i] == '\t') buffer[i] = ' ';

		lexer.stream = lexer.start;
		lex_line(&lexer, tokens);

		lexer.line += 1;
		lexer.col = 1;
	}

	struct Token end_of_file = {
		.type = TOK_EOF,
		.filename = filename,
		.line = lexer.line,
		.col = lexer.col,
	};

	vec_push(tokens, &end_of_file);

	if (lexer.errors)
		errx("too many errors");
}


// LEXER ERRORS //

void lexer_err(struct Lexer *lexer, enum LexerErrorType type, const char *offset, const char *fmt, ...) {
	// print location info
	int column = offset ? lexer->col - (lexer->stream - offset) : lexer->col;
	printf(WHITE"%s:%d:%d: ", lexer->filename, lexer->line, column);

	// print coloured error type
	switch (type) {
		case NOTE:
			printf(GREY "note: " RESET);
			break;

		case WARNING:
			printf(MAGENTA "warning: " RESET);
			break;

		case ERROR:
			printf(RED "error: " RESET);
			lexer->errors = true;
			break;
	}

	// print message
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	// print context
	//printf("\n%5d | %s\n", lexer->line, lexer->start);
	//printf("      | ");

	//int indent = (offset ?: lexer->stream) - lexer->start;
	//int length = offset ? (lexer->stream - offset - 1) : 0;

	//for (int i = 0; i < indent; i++) putchar(' ');
	//putchar('^');

	//for (int i = 0; i < length; i++) putchar('~');
	//putchar('\n');
	putchar('\n');
}
