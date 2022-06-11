#include "parser.h"
#include "ast.h"

#include <stddef.h>

static inline
struct Token *peek_next(struct Parser *parser) {
	return parser->length ? parser->tokens : NULL;
}

static inline
struct Token *chop_next(struct Parser *parser) {
	return parser->length--, parser->tokens++;
}


struct ExprNode *parse_expression(struct Parser *parser) {
	return NULL;
}
