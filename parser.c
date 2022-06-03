#include "parser.h"
#include "lexer.h"
#include "tokens.h"

#include "util/allocator.h"
#include "util/error.h"
#include "util/vec.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	MAX_BUFFER_SIZE = 1024,
	MAX_LINE_LENGTH = 120,
};

static
const char multichar[][2] = {
	"<<", ">>", "==", "!=", "<=", ">=", "&&", "||",
};

void parse_token(struct Lexer *lexer, struct Vec *tokens) {
	struct Token token = {
		.filename = lexer->filename,
		.line = lexer->line,
	};

	char buffer[MAX_BUFFER_SIZE];
	int length;

	switch (peek_next(lexer)) {
		case 'a' ... 'z':
		case 'A' ... 'Z':
			token.type = IDENTIFIER;
			length = chop_identifier(lexer, buffer);
			token.text = store_string(lexer->allocator, buffer, length);
			break;

		case '#':
			token.type = PREPROC;
			chop_next(lexer);

			length = chop_identifier(lexer, buffer);
			token.text = store_string(lexer->allocator, buffer, length);
			break;

		case '0' ... '9':
			token.type = INT;
			token.value = chop_int(lexer);
			break;

		case '"':
			token.type = STRING;

			length = chop_string(lexer, buffer);
			if  (length < 0) return; //string error

			token.text = store_string(lexer->allocator, buffer, length);
			break;

		default:
			token.type = PUNCTUATION;

			// handle multi-char punctuation
			int size = sizeof multichar / sizeof multichar[0];
			token.value = chop_next(lexer);

			char tmp = peek_next(lexer);

			for (int i = 0; i < size; i++) {
				if (multichar[i][0] == token.value &&
				    multichar[i][1] == tmp) {
					token.value *= tmp;
					chop_next(lexer);
					break;
				}
			}

			break;
	}

	vec_push(tokens, &token);
}


void parse_line(struct Lexer *lexer, struct Vec *tokens) {
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

		parse_token(lexer, tokens);
	}
}


void parse_file(const char *filename, struct Allocator *allocator, struct Vec *tokens) {
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

		if (length == MAX_BUFFER_SIZE - 1) {
			lexer_err(&lexer, ERROR, NULL, "line is too long");
		}

		else if (length > MAX_LINE_LENGTH) {
			lexer_err(&lexer, WARNING, NULL, "line exceeds %d chars", MAX_LINE_LENGTH);
		}

		lexer.stream = lexer.start;
		parse_line(&lexer, tokens);

		lexer.line += 1;
		lexer.col = 1;
	}

	if (!lexer.errors)
		return;

error:
	err("too many errors");
	exit(1);
}