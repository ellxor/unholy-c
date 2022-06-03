#ifndef ERROR_H_
#define ERROR_H_

#include "util.h"

void set_program(const char *prog);
void err(const char *fmt, ...) PRINTF(1,2);

#define internal_error() __internal_error(__FILE__, __LINE__)
void __internal_error(const char *filename, int line);

#endif