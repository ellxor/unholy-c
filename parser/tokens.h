#ifndef TOKENS_H_
#define TOKENS_H_

enum TokenType {
	INT,
	// TODO: add floating point literals (and to Token)
	STRING,
	PUNCTUATION,
	IDENTIFIER,
	KEYWORD,
	PREPROC,
};

struct Token {
	enum TokenType type;

	union {
		int value;
		const char *text;
	};

	// location reference
	const char *filename;
	int line;
};

enum MultiCharPunctuation {
	LEFT_SHIFT  = '<' * '<',
	RIGHT_SHIFT = '>' * '>',

	EQUAL       = '=' * '=',
	NOT_EQUAL   = '!' * '=',
	LESS_EQUAL  = '<' * '=',
	MORE_EQUAL  = '>' * '=',

	LOGICAL_AND = '&' * '&',
	LOGICAL_OR  = '|' * '|',

	INCREMENT   = '+' * '+',
	DECREMENT   = '-' * '-',

	SCOPE       = ':' * ':',

	// TODO: add assignment operators (+=, -=, ...)
};

#endif
