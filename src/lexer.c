#include "lexer.h"
#include "tokens.h"

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "util.h"

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
	else if ('a' <= c && c <= 'f')  return chop_next(lexer) - 'a' + 10;
	else if ('A' <= c && c <= 'F')  return chop_next(lexer) - 'A' + 10;

	lexer_err(lexer, ERROR, NULL, "expected hex digit, found `%c`", c);
	return chop_next(lexer);
}


static
int chop_int(struct Lexer *lexer) {
	unsigned result = 0, digits = 0, base = 10;
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

		unsigned old = result;

		result *= base;
		result += digit;

		if (result < old)
			overflow = true;
	}

	if (digits == 0) {
		lexer_err(lexer, ERROR, NULL, "expected digits after base specifier");
	}

	if (overflow) {
		int length = lexer->stream - start;
		lexer_err(lexer, ERROR, start, "integer constant overflows max int type: `%.*s`", length, start);
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

// KEYWORDS
struct KeywordEntry {
	const char *keyword;
	int length, id;
	unsigned hash;
};

#define SIZE 256

static_assert(KEYWORD_COUNT == 19, "update table to add/remove keyword");

static
struct KeywordEntry keywords[SIZE] = {
	[0xdf] = { .keyword = "break",      .length = 5,   KEYWORD_BREAK,      .hash = 0x64facedf },
	[0x1d] = { .keyword = "case",       .length = 4,   KEYWORD_CASE,       .hash = 0xf61c661d },
	[0xd8] = { .keyword = "do",         .length = 2,   KEYWORD_DO,         .hash = 0x54df01d8 },
	[0x27] = { .keyword = "else",       .length = 4,   KEYWORD_ELSE,       .hash = 0xc3248327 },
	[0x0a] = { .keyword = "enum",       .length = 4,   KEYWORD_ENUM,       .hash = 0x5acab70a },
	[0x2b] = { .keyword = "false",      .length = 5,   KEYWORD_FALSE,      .hash = 0x42f25c2b },
	[0x9a] = { .keyword = "if",         .length = 2,   KEYWORD_IF,         .hash = 0xa268309a },
	[0xa3] = { .keyword = "int",        .length = 3,   KEYWORD_INT,        .hash = 0xda41e7a3 },
	[0x5c] = { .keyword = "return",     .length = 6,   KEYWORD_RETURN,     .hash = 0xb217c05c },
	[0x6b] = { .keyword = "sizeof",     .length = 6,   KEYWORD_SIZEOF,     .hash = 0xb6ffd66b },
	[0x07] = { .keyword = "struct",     .length = 6,   KEYWORD_STRUCT,     .hash = 0x029c0107 },
	[0x47] = { .keyword = "switch",     .length = 6,   KEYWORD_SWITCH,     .hash = 0x11b08047 },
	[0x9d] = { .keyword = "true",       .length = 4,   KEYWORD_TRUE,       .hash = 0x506b889d },
	[0x7b] = { .keyword = "union",      .length = 5,   KEYWORD_UNION,      .hash = 0xf077d47b },
	[0x23] = { .keyword = "u8",         .length = 2,   KEYWORD_U8,         .hash = 0x06f4f723 },
	[0x31] = { .keyword = "u16",        .length = 3,   KEYWORD_U16,        .hash = 0x33d14a31 },
	[0x60] = { .keyword = "u32",        .length = 3,   KEYWORD_U32,        .hash = 0x3f0e5960 },
	[0xa7] = { .keyword = "void",       .length = 4,   KEYWORD_VOID,       .hash = 0x85859ea7 },
	[0xd6] = { .keyword = "while",      .length = 5,   KEYWORD_WHILE,      .hash = 0xe80392d6 },
};

static_assert(PREPROC_COUNT == 7, "updated table to add/remove preprocessor directive");

static
struct KeywordEntry preprocs[] = {
	[0x15] = { .keyword = "elif",       .length = 4,   PREPROC_ELIF,       .hash = 0x39112d15 },
	[0x27] = { .keyword = "else",       .length = 4,   PREPROC_ELSE,       .hash = 0xc3248327 },
	[0xe1] = { .keyword = "endif",      .length = 5,   PREPROC_ENDIF,      .hash = 0x6688b4e1 },
	[0xa3] = { .keyword = "error",      .length = 5,   PREPROC_ERROR,      .hash = 0x3baebaa3 },
	[0x9a] = { .keyword = "if",         .length = 2,   PREPROC_IF,         .hash = 0xa268309a },
	[0xd3] = { .keyword = "include",    .length = 7,   PREPROC_INCLUDE,    .hash = 0x0ad7a1d3 },
	[0x7d] = { .keyword = "warning",    .length = 7,   PREPROC_WARNING,    .hash = 0x6b8db27d },
};

static
enum TokenType lookup_keyword(const char *in, int length, enum TokenType type) {
	assert(type == KEYWORD || type == PREPROC);

	struct KeywordEntry *table = (type == KEYWORD) ? keywords : preprocs;

	unsigned key = hash(in, length);
	unsigned idx = key & (SIZE - 1);

	struct KeywordEntry entry = table[idx];

	if (entry.length == length && entry.hash == key) {
		if (strncmp(entry.keyword, in, length) == 0) {
			return entry.id;
		}
	}

	return NONE;
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
				token.type = SYMBOL;
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
			token.type = INT_LITERAL;
			token.value = chop_int(lexer);
			break;

		case '"':
			token.type = STRING_LITERAL;

			length = chop_string(lexer, '"', buffer);
			if (length < 0) return; //string error

			token.text = store_string(lexer->allocator, buffer, length);
			break;

		case '\'':
			token.type = INT_LITERAL;

			length = chop_string(lexer, '\'', buffer);
			if (length < 0) return; //string error

			if (length > 1) {
				lexer_err(lexer, ERROR, lexer->stream - length - 2, "character constant is more than 1 character");
			}

			token.value = *buffer;
			token.is_char = true;
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
		.errors = 0,
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

	if (lexer.errors > 0)
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
			lexer->errors++;
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
