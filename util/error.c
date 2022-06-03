#include "error.h"
#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const char *program = NULL;

void set_program(const char *prog) {
	program = prog;
}

void err(const char *fmt, ...) {
	printf(WHITE "%s: " RED "error: " RESET, program);

	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);
	putchar('\n');
	va_end(args);
}

void __internal_error(const char *filename, int line) {
	printf(WHITE "%s@%s:%d: " RED "error: " RESET "internal compiler error",
		program, filename, line);
	abort();
}