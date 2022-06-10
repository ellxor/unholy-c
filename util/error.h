#ifndef ERROR_H_
#define ERROR_H_

#include "util.h"
#include <stdnoreturn.h>

void set_program(const char *prog);
noreturn void errx(const char *fmt, ...) PRINTF(1,2);

#define internal_error(fmt, ...) __internal_error(__FILE__, __LINE__, fmt, __VA_ARGS__)
noreturn void __internal_error(const char *filename, int line, const char *fmt, ...) PRINTF(3,4);

#endif
