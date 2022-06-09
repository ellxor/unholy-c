#include "lexer.h"
#include "tokens.h"

#include "../util/allocator.h"
#include "../util/error.h"
#include "../util/vec.h"
#include "../util/hash.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	MAX_BUFFER_SIZE = 1024,
	MAX_LINE_LENGTH = 120,
};

static
const char multichar[][2] = {
	"<<", ">>", "==", "!=", "<=", ">=", "&&", "||", "++", "--", "::", "..",
};

void chop_token(struct Lexer *lexer, struct Vec *tokens) {
	struct Token token = {
		.filename = lexer->filename,
		.line = lexer->line,
	};

	char buffer[MAX_BUFFER_SIZE];
	int length;

	switch (peek_next(lexer)) {
		case 'a' ... 'z':
		case 'A' ... 'Z':
			length = chop_identifier(lexer, buffer);
			enum TokenType type = lookup_keyword(buffer, length, KEYWORD);

			if (type == KEYWORD_NONE) {
				token.type = IDENTIFIER;
				token.text = store_string(lexer->allocator, buffer, length);
			} else {
				token.type = type;
			}

			break;

		case '#':
			chop_next(lexer);

			length = chop_identifier(lexer, buffer);
			token.type = lookup_keyword(buffer, length, PREPROC);

			if (token.type == PREPROC_NONE) {
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

		default:
			token.type = PUNCTUATION;

			// handle multi-char punctuation
			int size = sizeof multichar / sizeof multichar[0];

			char a = chop_next(lexer);
			char b = peek_next(lexer);

			// store single character temporarily
			token.value = a;

			for (int i = 0; i < size; i++) {
				if (multichar[i][0] == a && multichar[i][1] == b) {
					token.value = multichar_mix(a,b);
					chop_next(lexer);
					break;
				}
			}

			// handle invalid ascii chars
			if (token.value <= CHAR_MAX) {
				switch (token.value) {
					case 0 ... 0x20:
					case 127: // DEL
					case '$':
					case '@':
					case '`':
					case '\\':
						lexer_err(lexer, ERROR, lexer->stream - 1, "invalid ascii char in source file");
						break;

					case '_':
						lexer_err(lexer, ERROR, lexer->stream - 1, "identifiers cannot begin with an underscore");
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
		err("file `%s` not found", filename);
		goto error;
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
		for (int i = 0; i < length; i++)
			if (buffer[i] == '\t') buffer[i] = ' ';

		lexer.stream = lexer.start;
		lex_line(&lexer, tokens);

		lexer.line += 1;
		lexer.col = 1;
	}

	if (!lexer.errors)
		return;

error:
	err("too many errors");
	exit(1);
}
