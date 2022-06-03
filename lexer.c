#include "lexer.h"

#include "util/error.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

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

	if ('0' <= c && c <= '9')	return chop_next(lexer) - '0';
	else if ('a' <= c && c <= 'f')  return chop_next(lexer) - 'a';
	else if ('A' <= c && c <= 'F')	return chop_next(lexer) - 'A';

	lexer_err(lexer, ERROR, NULL, "expected hex digit, found `%c`", c);
	return chop_next(lexer);
}


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

	while (isalnum(peek_next(lexer)) || peek_next(lexer) == '\'') {
		// ignore digit seperator
		if (peek_next(lexer) == '\'') {
			chop_next(lexer);
			continue;
		}

		char digit = (base == 16) ? chop_hex_digit(lexer) : chop_digit(lexer);
		digits++;

		if (base == 2 && digit >= 2) {
			lexer_err(lexer, ERROR, lexer->stream - 1, "binary digit is not [01]");
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
		lexer_err(lexer, WARNING, start, "integer literal overflows int type");
		lexer_err(lexer, NOTE, start, "integer literal has value of `%d`", result);
	}

	return result;
}


int chop_string(struct Lexer *lexer, char *buffer) {
	// keep copy of base string
	const char *start = lexer->stream;
	int length = 0;

	// strip leading quote
	chop_next(lexer);

	while (peek_next(lexer) != '"') {
		char c = chop_next(lexer);

		if (c == '\0') {
			lexer_err(lexer, ERROR, start, "unterminated string literal");
			return -1;
		}

		// escape sequences
		if (c == '\\') {
			char escape = chop_next(lexer);

			switch (escape) {
				case 'e': c = '\e'; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'v': c = '\v'; break;
				case '"': c = '\"'; break;
				case '\\': break;

				// TODO: implement hex/unicode code points
				case 'x': lexer_err(lexer, ERROR, NULL, "hex code points are not implemented yet!"); break;
				case 'u': lexer_err(lexer, ERROR, NULL, "unicode code points are not implemented yet!"); break;
				case 'U': lexer_err(lexer, ERROR, NULL, "unicode code points are not implemented yet!"); break;

				case 0:
					lexer_err(lexer, ERROR, start, "unterminated string literal");
					return -1;

				default:
					lexer_err(lexer, WARNING, lexer->stream - 1, "invalid escape sequence");
					lexer_err(lexer, NOTE, lexer->stream - 2, "`\\` character will be ignored");
					c = escape;
			}
		}

		*buffer++ = c;
		length++;
	}

	// strip trailing quote
	chop_next(lexer);

	return length;
}


int chop_identifier(struct Lexer *lexer, char *buffer) {
	int length = 0;

	while (isalnum(peek_next(lexer)) || peek_next(lexer) == '_') {
		char c = chop_next(lexer);
		*buffer++ = c;
		length++;
	}

	return length;
}