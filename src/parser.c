#include "parser.h"

#include "ast.h"
#include "tokens.h"
#include "util/allocator.h"
#include "util/error.h"

#include <stddef.h>

static inline
struct Token *peek_next(struct Parser *parser) {
	return parser->length ? parser->tokens : NULL;
}

static inline
struct Token *chop_next(struct Parser *parser) {
	return parser->length--, parser->tokens++;
}


static
int precedence[] = {
	[')'] = -1,
	['+'] = 1,
	['-'] = 1,
	['*'] = 2,
	['/'] = 2,
	['%'] = 2,
};


struct ExprNode *parse_expression(struct Parser *parser, int min_precedence) {
	if (peek_next(parser) == NULL) {
		errx("expected token!");
	}

	struct ExprNode *lhs = NULL;

	if (peek_next(parser)->type == PUNCTUATION && peek_next(parser)->value == '(') {
		chop_next(parser); // remove (
		lhs = parse_expression(parser, -1);
		chop_next(parser); // remove )
	}

	else {
		struct ExprNode term = { chop_next(parser), TERM, NULL, NULL };
		lhs = store_object(parser->allocator, &term, sizeof term);
	}

	while (peek_next(parser) != NULL && precedence[peek_next(parser)->value] > min_precedence) {
		struct Token *op = chop_next(parser);

		struct ExprNode operator = { op, BINARY_OP, lhs, NULL };
		operator.rhs = parse_expression(parser, precedence[op->value]);

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	return lhs;
}
