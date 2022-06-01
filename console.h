#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "lexer.h"

enum ErrorType {
	NOTE,
	WARNING,
	ERROR,
};

#if defined(__GNUC__) || defined(__clang__)
	__attribute__((format (printf, 3, 4)))
#endif

void msg(struct Lexer *lexer, enum ErrorType, const char *fmt, ...);

#endif //CONSOLE_H_