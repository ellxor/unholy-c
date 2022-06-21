#ifndef PARSER_H_
#define PARSER_H_

#include "allocator.h"
#include "ast.h"
#include "tokens.h"

#include <stdbool.h>

struct Parser {
	struct Token *tokens;
	int length;

	struct Allocator *allocator;
	int errors;
};

struct AST_Expression *parse_expression(struct Parser *);
//struct DeclNode *parse_declaration(struct Parser *);

#endif //PARSER_H_
