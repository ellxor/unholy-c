#include <stdio.h>

#include "console.h"
#include "lexer.h"

int main() {
	struct Lexer lexer = {
		.filename = "<builtin>",
		.stream = NULL,
		.start = "\"\\abc\\xff\"",
		.line = 1,
		.col = 1,
		.errors = 0
	};

	lexer.stream = lexer.start;

	char buffer[1024];
	int x = chop_string(&lexer, buffer);

	printf("PARSER: %d, `%.*s`\n", x, x, buffer);
}