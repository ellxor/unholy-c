#include <stdio.h>

#include "console.h"
#include "lexer.h"

int main() {
	struct Lexer lexer = {
		.filename = "<builtin>",
		.stream = "hello world",
		.reference = NULL,
		.line = 1,
		.col = 1,
		.errors = 0
	};

	lexer.reference = lexer.stream;
	lexer.stream += 6;

	msg(&lexer, ERROR, "`world` is undefined");
}