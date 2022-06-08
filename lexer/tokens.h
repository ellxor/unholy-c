#ifndef TOKENS_H_
#define TOKENS_H_

// update when modifying keyword and preproc enums
#define KEYWORD_COUNT 29
#define PREPROC_COUNT 11

enum TokenType {
	// literals :: TODO: add floating point literals (and to Token)
	INT,
	FLOAT,
	STRING,

	PUNCTUATION,
	IDENTIFIER,

	KEYWORD,
	KEYWORD_NONE,
	KEYWORD_BOOL,
	KEYWORD_BREAK,
	KEYWORD_CASE,
	KEYWORD_CHAR,
	KEYWORD_CONST,
	KEYWORD_CONTINUE,
	KEYWORD_DO,
	KEYWORD_ELSE,
	KEYWORD_ENUM,
	KEYWORD_FALSE,
	KEYWORD_FOR,
	KEYWORD_GOTO,
	KEYWORD_IF,
	KEYWORD_INT,
	KEYWORD_I8,
	KEYWORD_I16,
	KEYWORD_I32,
	KEYWORD_RETURN,
	KEYWORD_SIZEOF,
	KEYWORD_STATIC,
	KEYWORD_STRUCT,
	KEYWORD_SWITCH,
	KEYWORD_TRUE,
	KEYWORD_UNION,
	KEYWORD_U8,
	KEYWORD_U16,
	KEYWORD_U32,
	KEYWORD_VOID,
	KEYWORD_WHILE,

	PREPROC,
	PREPROC_NONE,
	PREPROC_ASSERT,
	PREPROC_DEFINE,
	PREPROC_ELIF,
	PREPROC_ELSE,
	PREPROC_EMBED,
	PREPROC_ENDIF,
	PREPROC_ERROR,
	PREPROC_IF,
	PREPROC_INCLUDE,
	PREPROC_PRAGMA,
	PREPROC_WARNING,
};

struct Token {
	enum TokenType type;

	union { int value, length; };
	const char *text;

	// location reference
	const char *filename;
	int line;
};

enum Punctuation {
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
};

#endif
