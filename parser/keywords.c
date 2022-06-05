#include "keywords.h"
#include "tokens.h"

#include <assert.h>
#include <string.h>
#include "../util/hash.h"

struct KeywordEntry {
	const char *keyword;
	int length, id;

	unsigned hash;
};

enum { SIZE = 256 };

struct KeywordEntry keywords[SIZE] = {
	[0xdf] = { .keyword = "break",      .length = 5,   KEYWORD_BREAK,      .hash = 0x64facedf },
	[0x1d] = { .keyword = "case",       .length = 4,   KEYWORD_CASE,       .hash = 0xf61c661d },
	[0x71] = { .keyword = "char",       .length = 4,   KEYWORD_CHAR,       .hash = 0x09763b71 },
	[0xdd] = { .keyword = "const",      .length = 5,   KEYWORD_CONST,      .hash = 0xe485d9dd },
	[0xe3] = { .keyword = "continue",   .length = 8,   KEYWORD_CONTINUE,   .hash = 0x2c4a0ce3 },
	[0xd8] = { .keyword = "do",         .length = 2,   KEYWORD_DO,         .hash = 0x54df01d8 },
	[0x27] = { .keyword = "else",       .length = 4,   KEYWORD_ELSE,       .hash = 0xc3248327 },
	[0x0a] = { .keyword = "enum",       .length = 4,   KEYWORD_ENUM,       .hash = 0x5acab70a },
	[0xdb] = { .keyword = "for",        .length = 3,   KEYWORD_FOR,        .hash = 0x0dc848db },
	[0x97] = { .keyword = "goto",       .length = 4,   KEYWORD_GOTO,       .hash = 0x5d1eb997 },
	[0x9a] = { .keyword = "if",         .length = 2,   KEYWORD_IF,         .hash = 0xa268309a },
	[0xa3] = { .keyword = "int",        .length = 3,   KEYWORD_INT,        .hash = 0xda41e7a3 },
	[0x0c] = { .keyword = "i8",         .length = 2,   KEYWORD_I8,         .hash = 0xa544510c },
	[0xb6] = { .keyword = "i16",        .length = 3,   KEYWORD_I16,        .hash = 0x476416b6 },
	[0x84] = { .keyword = "i32",        .length = 3,   KEYWORD_I32,        .hash = 0x20e4d084 },
	[0x5c] = { .keyword = "return",     .length = 6,   KEYWORD_RETURN,     .hash = 0xb217c05c },
	[0x6b] = { .keyword = "sizeof",     .length = 6,   KEYWORD_SIZEOF,     .hash = 0xb6ffd66b },
	[0x39] = { .keyword = "static",     .length = 6,   KEYWORD_STATIC,     .hash = 0x5bad8839 },
	[0x07] = { .keyword = "struct",     .length = 6,   KEYWORD_STRUCT,     .hash = 0x029c0107 },
	[0x47] = { .keyword = "switch",     .length = 6,   KEYWORD_SWITCH,     .hash = 0x11b08047 },
	[0x7b] = { .keyword = "union",      .length = 5,   KEYWORD_UNION,      .hash = 0xf077d47b },
	[0x23] = { .keyword = "u8",         .length = 2,   KEYWORD_U8,         .hash = 0x06f4f723 },
	[0x31] = { .keyword = "u16",        .length = 3,   KEYWORD_U16,        .hash = 0x33d14a31 },
	[0x60] = { .keyword = "u32",        .length = 3,   KEYWORD_U32,        .hash = 0x3f0e5960 },
	[0xa7] = { .keyword = "void",       .length = 4,   KEYWORD_VOID,       .hash = 0x85859ea7 },
	[0xd6] = { .keyword = "while",      .length = 5,   KEYWORD_WHILE,      .hash = 0xe80392d6 },
};

struct KeywordEntry preprocs[SIZE] = {
	[0xfa] = { .keyword = "assert",     .length = 6,   PREPROC_ASSERT,     .hash = 0x12f3cefa },
	[0x74] = { .keyword = "define",     .length = 6,   PREPROC_DEFINE,     .hash = 0x4406fb74 },
	[0xe1] = { .keyword = "endif",      .length = 5,   PREPROC_ENDIF,      .hash = 0x6688b4e1 },
	[0x15] = { .keyword = "elif",       .length = 4,   PREPROC_ELIF,       .hash = 0x39112d15 },
	[0x27] = { .keyword = "else",       .length = 4,   PREPROC_ELSE,       .hash = 0xc3248327 },
	[0xa3] = { .keyword = "error",      .length = 5,   PREPROC_ERROR,      .hash = 0x3baebaa3 },
	[0x9a] = { .keyword = "if",         .length = 2,   PREPROC_IF,         .hash = 0xa268309a },
	[0xd3] = { .keyword = "include",    .length = 7,   PREPROC_INCLUDE,    .hash = 0x0ad7a1d3 },
	[0x8e] = { .keyword = "pragma",     .length = 6,   PREPROC_PRAGMA,     .hash = 0xd30a808e },
	[0x7d] = { .keyword = "warning",    .length = 7,   PREPROC_WARNING,    .hash = 0x6b8db27d },
};

int parse_keyword(const char *in, int length, enum TokenType type) {
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

	return -1;
}