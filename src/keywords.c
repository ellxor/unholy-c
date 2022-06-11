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

enum { SIZE = 1 << 10 };

static_assert(KEYWORD_COUNT == 29, "update table to add/remove keyword");

struct KeywordEntry keywords[SIZE] = {
	[0x058] = { .keyword = "bool",       .length = 4,   KEYWORD_BOOL,       .hash = 0x7050f458 },
	[0x1cd] = { .keyword = "break",      .length = 5,   KEYWORD_BREAK,      .hash = 0x2c08d1cd },
	[0x2a4] = { .keyword = "case",       .length = 4,   KEYWORD_CASE,       .hash = 0x662b4ea4 },
	[0x2be] = { .keyword = "char",       .length = 4,   KEYWORD_CHAR,       .hash = 0xec9eb6be },
	[0x15a] = { .keyword = "const",      .length = 5,   KEYWORD_CONST,      .hash = 0x90d4855a },
	[0x343] = { .keyword = "continue",   .length = 8,   KEYWORD_CONTINUE,   .hash = 0x65303343 },
	[0x349] = { .keyword = "do",         .length = 2,   KEYWORD_DO,         .hash = 0x04b1f349 },
	[0x0a5] = { .keyword = "else",       .length = 4,   KEYWORD_ELSE,       .hash = 0xa7fbd4a5 },
	[0x321] = { .keyword = "enum",       .length = 4,   KEYWORD_ENUM,       .hash = 0x8c2c4721 },
	[0x21d] = { .keyword = "false",      .length = 5,   KEYWORD_FALSE,      .hash = 0xfb67661d },
	[0x128] = { .keyword = "for",        .length = 3,   KEYWORD_FOR,        .hash = 0x8cdee928 },
	[0x072] = { .keyword = "goto",       .length = 4,   KEYWORD_GOTO,       .hash = 0xa63f7072 },
	[0x08a] = { .keyword = "if",         .length = 2,   KEYWORD_IF,         .hash = 0x20db688a },
	[0x225] = { .keyword = "int",        .length = 3,   KEYWORD_INT,        .hash = 0x05013225 },
	[0x29b] = { .keyword = "i8",         .length = 2,   KEYWORD_I8,         .hash = 0x796a429b },
	[0x3b1] = { .keyword = "i16",        .length = 3,   KEYWORD_I16,        .hash = 0x756dafb1 },
	[0x2ae] = { .keyword = "i32",        .length = 3,   KEYWORD_I32,        .hash = 0x81c91aae },
	[0x2a8] = { .keyword = "return",     .length = 6,   KEYWORD_RETURN,     .hash = 0x798f76a8 },
	[0x15f] = { .keyword = "sizeof",     .length = 6,   KEYWORD_SIZEOF,     .hash = 0xfeac6d5f },
	[0x220] = { .keyword = "static",     .length = 6,   KEYWORD_STATIC,     .hash = 0xf9b36e20 },
	[0x2ab] = { .keyword = "struct",     .length = 6,   KEYWORD_STRUCT,     .hash = 0x6440aeab },
	[0x246] = { .keyword = "switch",     .length = 6,   KEYWORD_SWITCH,     .hash = 0x8d49f646 },
	[0x2ea] = { .keyword = "true",       .length = 4,   KEYWORD_TRUE,       .hash = 0x7ec2d6ea },
	[0x04e] = { .keyword = "union",      .length = 5,   KEYWORD_UNION,      .hash = 0x5e09144e },
	[0x11e] = { .keyword = "u8",         .length = 2,   KEYWORD_U8,         .hash = 0xb8f5f51e },
	[0x2a5] = { .keyword = "u16",        .length = 3,   KEYWORD_U16,        .hash = 0x51cdf6a5 },
	[0x398] = { .keyword = "u32",        .length = 3,   KEYWORD_U32,        .hash = 0x10e50798 },
	[0x03d] = { .keyword = "void",       .length = 4,   KEYWORD_VOID,       .hash = 0x641be03d },
	[0x33d] = { .keyword = "while",      .length = 5,   KEYWORD_WHILE,      .hash = 0x2ad1f73d },
};

static_assert(PREPROC_COUNT == 11, "updated table to add/remove preprocessor directive");

struct KeywordEntry preprocs[SIZE] = {
	[0x2ba] = { .keyword = "assert",     .length = 6,   PREPROC_ASSERT,     .hash = 0x6a9c4aba },
	[0x2c8] = { .keyword = "define",     .length = 6,   PREPROC_DEFINE,     .hash = 0xd4923ec8 },
	[0x218] = { .keyword = "elif",       .length = 4,   PREPROC_ELIF,       .hash = 0xf7609218 },
	[0x0a5] = { .keyword = "else",       .length = 4,   PREPROC_ELSE,       .hash = 0xa7fbd4a5 },
	[0x3cb] = { .keyword = "embed",      .length = 5,   PREPROC_EMBED,      .hash = 0x27b233cb },
	[0x02b] = { .keyword = "endif",      .length = 5,   PREPROC_ENDIF,      .hash = 0x5a2bd42b },
	[0x1bc] = { .keyword = "error",      .length = 5,   PREPROC_ERROR,      .hash = 0xe0a6c9bc },
	[0x08a] = { .keyword = "if",         .length = 2,   PREPROC_IF,         .hash = 0x20db688a },
	[0x29e] = { .keyword = "include",    .length = 7,   PREPROC_INCLUDE,    .hash = 0x16ef029e },
	[0x1f7] = { .keyword = "pragma",     .length = 6,   PREPROC_PRAGMA,     .hash = 0x1c721df7 },
	[0x31d] = { .keyword = "warning",    .length = 7,   PREPROC_WARNING,    .hash = 0x9a72db1d },
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
