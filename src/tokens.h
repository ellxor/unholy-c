#ifndef TOKENS_H_
#define TOKENS_H_

#include <stdbool.h>

// update when modifying keyword and preproc enums
#define KEYWORD_COUNT (KEYWORD_END - KEYWORD - 1)
#define PREPROC_COUNT (PREPROC_END - PREPROC - 1)

enum TokenType {
	INT_LITERAL,
	//FLOAT_LITERAL,
	STRING_LITERAL,
	PUNCTUATION,
	SYMBOL,

	KEYWORD,
	KEYWORD_BOOL,
	KEYWORD_BREAK,
	KEYWORD_CASE,
	KEYWORD_CHAR,
	KEYWORD_CONTINUE,
	KEYWORD_DO,
	KEYWORD_ELSE,
	KEYWORD_ENUM,
	KEYWORD_FALSE,
	KEYWORD_IF,
	KEYWORD_INT,
	KEYWORD_RETURN,
	KEYWORD_SIZEOF,
	KEYWORD_STRUCT,
	KEYWORD_SWITCH,
	KEYWORD_TRUE,
	KEYWORD_UNION,
	KEYWORD_U32,
	KEYWORD_VOID,
	KEYWORD_WHILE,
	KEYWORD_END,

	PREPROC,
	PREPROC_ELIF,
	PREPROC_ELSE,
	PREPROC_ENDIF,
	PREPROC_ERROR,
	PREPROC_IF,
	PREPROC_INCLUDE,
	PREPROC_WARNING,
	PREPROC_END,

	TOK_EOF,
	NONE,
};

struct Token {
	enum TokenType type;

	// data: TODO: add support for floating point literals
	union {
		struct { unsigned value; bool is_char; };
		struct { int length; const char *text; };
		struct { int pointers; };
	};

	// location reference
	const char *filename;
	int line, col;
};

// multi-character punctuation:
//
// multi character symbols are hashed using the formula:
// "ab" => mix('a', 'b')
//
// the result of this formula should be in the range
// (128...255) were no other 7-bit ASCII can be
//

// single lea instruction
#define multichar_mix(a,b) (((a) + 2*(b) + 14) & 0xff)

enum MultiChar {
	INC = multichar_mix('+','+'),
	DEC = multichar_mix('-','-'),

	SHL = multichar_mix('<','<'),
	SHR = multichar_mix('>','>'),

	EQ  = multichar_mix('=','='),
	NEQ = multichar_mix('!','='),
	LEQ = multichar_mix('<','='),
	GEQ = multichar_mix('>','='),

	AND = multichar_mix('&','&'),
	OR  = multichar_mix('|','|'),

	COM = multichar_mix(':', ':'),

	POST_INC = INC + 1,
	POST_DEC = DEC + 1,
};

#endif
