#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "lexer.h"
#include <stddef.h>

enum ErrorType {
	NOTE,
	WARNING,
	ERROR,
};

#if defined(__GNUC__) || defined(__clang__)
	__attribute__((format (printf, 4, 5)))
#endif

void msg(struct Lexer *lexer, enum ErrorType, const char *offset, const char *fmt, ...);

#endif //CONSOLE_H_