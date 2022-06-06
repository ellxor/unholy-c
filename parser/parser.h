#ifndef PARSER_H_
#define PARSER_H_

#include "lexer.h"
#include "tokens.h"
#include "../util/allocator.h"
#include "../util/vec.h"

void parse_line(struct Lexer *, struct Vec *tokens);
void parse_file(const char *filename, struct Allocator *, struct Vec *tokens);

#endif //PARSER_H_
