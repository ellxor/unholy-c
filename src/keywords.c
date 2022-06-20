#include "lexer.h"
#include "tokens.h"

#include <assert.h>
#include <string.h>

#include "util/hash.h"

#if !defined(static_assert)
#define static_assert(cond, msg)
#endif

struct KeywordEntry {
	const char *keyword;
	int length, id;
	unsigned hash;
};

#define SIZE 512

static_assert(KEYWORD_COUNT == 20, "update table to add/remove keyword");

struct KeywordEntry keywords[SIZE] = {
	[0x058] = { .keyword = "bool",       .length = 4,   KEYWORD_BOOL,       .hash = 0x7050f458 },
	[0x1cd] = { .keyword = "break",      .length = 5,   KEYWORD_BREAK,      .hash = 0x2c08d1cd },
	[0x0a4] = { .keyword = "case",       .length = 4,   KEYWORD_CASE,       .hash = 0x662b4ea4 },
	[0x0be] = { .keyword = "char",       .length = 4,   KEYWORD_CHAR,       .hash = 0xec9eb6be },
	[0x143] = { .keyword = "continue",   .length = 8,   KEYWORD_CONTINUE,   .hash = 0x65303343 },
	[0x149] = { .keyword = "do",         .length = 2,   KEYWORD_DO,         .hash = 0x04b1f349 },
	[0x0a5] = { .keyword = "else",       .length = 4,   KEYWORD_ELSE,       .hash = 0xa7fbd4a5 },
	[0x121] = { .keyword = "enum",       .length = 4,   KEYWORD_ENUM,       .hash = 0x8c2c4721 },
	[0x01d] = { .keyword = "false",      .length = 5,   KEYWORD_FALSE,      .hash = 0xfb67661d },
	[0x08a] = { .keyword = "if",         .length = 2,   KEYWORD_IF,         .hash = 0x20db688a },
	[0x025] = { .keyword = "int",        .length = 3,   KEYWORD_INT,        .hash = 0x05013225 },
	[0x0a8] = { .keyword = "return",     .length = 6,   KEYWORD_RETURN,     .hash = 0x798f76a8 },
	[0x15f] = { .keyword = "sizeof",     .length = 6,   KEYWORD_SIZEOF,     .hash = 0xfeac6d5f },
	[0x0ab] = { .keyword = "struct",     .length = 6,   KEYWORD_STRUCT,     .hash = 0x6440aeab },
	[0x046] = { .keyword = "switch",     .length = 6,   KEYWORD_SWITCH,     .hash = 0x8d49f646 },
	[0x0ea] = { .keyword = "true",       .length = 4,   KEYWORD_TRUE,       .hash = 0x7ec2d6ea },
	[0x04e] = { .keyword = "union",      .length = 5,   KEYWORD_UNION,      .hash = 0x5e09144e },
	[0x198] = { .keyword = "u32",        .length = 3,   KEYWORD_U32,        .hash = 0x10e50798 },
	[0x03d] = { .keyword = "void",       .length = 4,   KEYWORD_VOID,       .hash = 0x641be03d },
	[0x13d] = { .keyword = "while",      .length = 5,   KEYWORD_WHILE,      .hash = 0x2ad1f73d },
};

static_assert(PREPROC_COUNT == 7, "updated table to add/remove preprocessor directive");

struct KeywordEntry preprocs[SIZE] = {
	[0x018] = { .keyword = "elif",       .length = 4,   PREPROC_ELIF,       .hash = 0xf7609218 },
	[0x0a5] = { .keyword = "else",       .length = 4,   PREPROC_ELSE,       .hash = 0xa7fbd4a5 },
	[0x02b] = { .keyword = "endif",      .length = 5,   PREPROC_ENDIF,      .hash = 0x5a2bd42b },
	[0x1bc] = { .keyword = "error",      .length = 5,   PREPROC_ERROR,      .hash = 0xe0a6c9bc },
	[0x08a] = { .keyword = "if",         .length = 2,   PREPROC_IF,         .hash = 0x20db688a },
	[0x09e] = { .keyword = "include",    .length = 7,   PREPROC_INCLUDE,    .hash = 0x16ef029e },
	[0x11d] = { .keyword = "warning",    .length = 7,   PREPROC_WARNING,    .hash = 0x9a72db1d },
};

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
