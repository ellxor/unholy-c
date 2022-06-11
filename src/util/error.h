#ifndef ERROR_H_
#define ERROR_H_

#include "util.h"
#include <stdnoreturn.h>

void set_program(const char *prog);
noreturn void errx(const char *fmt, ...) PRINTF(1,2);

#endif
