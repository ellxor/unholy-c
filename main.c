#include <stdio.h>

#include "parser/lexer.h"
#include "parser/parser.h"
#include "parser/tokens.h"

#include "util/allocator.h"

void dump_token(struct Token token) {
	switch (token.type) {
		case INT:		printf("INT(%d)\n", token.value); break;
		case STRING:		printf("STRING(%s)\n", token.text); break;
		case IDENTIFIER:	printf("IDENT(%s)\n", token.text); break;
		case PREPROC:		printf("PREPROC(%d)\n", token.value); break;
		case KEYWORD: 		printf("KEYWORD(%d)\n", token.value); break;

		case PUNCTUATION:
			if (token.value > 0xff)
				printf("PUNC(%d)\n", token.value);
			else
				printf("PUNC(`%c`)\n", token.value);
			break;
	}
}

int main() {
	struct Vec tokens = vec(struct Token);
	struct Allocator allocator = init_allocator();

	parse_file("test", &allocator, &tokens);
	puts("\n!!! PARSING DONE !!!\n");

	struct Token *buffer = tokens.mem;
	int count = tokens.length;

	for (int i = 0; i < count; i++) {
		struct Token token = buffer[i];
		dump_token(token);
	}
}