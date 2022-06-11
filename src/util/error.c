#include "error.h"
#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

const char *program = NULL;

void set_program(const char *prog) {
	program = prog;
}

void errx(const char *fmt, ...) {
	printf(WHITE "%s: " RED "error: " RESET, program);

	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);
	va_end(args);

	putchar('\n');
	exit(1);
}
