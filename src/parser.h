#ifndef PARSER_H_
#define PARSER_H_

#include "ast.h"
#include "tokens.h"

#include "util/allocator.h"
#include "util/vec.h"

struct Parser {
	struct Token *tokens;
	int length;

	struct Allocator *allocator;
};

struct ExprNode *parse_expression(struct Parser *);

#endif //PARSER_H_
