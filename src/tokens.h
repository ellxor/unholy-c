#ifndef TOKENS_H_
#define TOKENS_H_

// update when modifying keyword and preproc enums
#define KEYWORD_COUNT 29
#define PREPROC_COUNT 11

enum TokenType {
	INT,
	FLOAT,
	STRING,
	PUNCTUATION,
	IDENTIFIER,

	KEYWORD,
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

	NONE,
};

struct Token {
	enum TokenType type;

	// data: TODO: add support for floating point literals
	union { int value, length, pointers; };
	const char *text;

	// location reference
	const char *filename;
	int line;
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
	SHL = multichar_mix('<','<'),
	SHR = multichar_mix('>','>'),

	EQ  = multichar_mix('=','='),
	NEQ = multichar_mix('!','='),
	LEQ = multichar_mix('<','='),
	GEQ = multichar_mix('>','='),

	AND = multichar_mix('&','&'),
	OR  = multichar_mix('|','|'),

	INC = multichar_mix('+','+'),
	DEC = multichar_mix('-','-'),

	SCOPE = multichar_mix(':',':'),
	RANGE = multichar_mix('.','.'),
};

#endif
