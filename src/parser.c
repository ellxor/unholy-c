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

#define MIN_PRECEDENCE -1

static
int precedence[] = {
	// `;` and `)` always end expressions
	[')'] = MIN_PRECEDENCE,
	[';'] = MIN_PRECEDENCE,

	// comma operator
	[','] = 0,

	// logical operators
	[OR]  = 3, [AND] = 4,

	// bitwise operators
	['|'] = 5, ['^'] = 6, ['&'] = 7,

	// comparsion operators
	[EQ]  = 8, [NEQ] = 8,
	['>'] = 9, ['<'] = 9, [LEQ] = 9, [GEQ] = 9,

	// shift operators
	[SHL] = 10, [SHR] = 10,

	// arithmetic operators
	['+'] = 11, ['-'] = 11,
	['*'] = 12, ['/'] = 12, ['%'] = 12,

	// unary operators
	[PRE_UNARY_OP] = 13,
	[POST_UNARY_OP] = 14,

	// member access
	['.'] = 14,
	[SCOPE] = 14,
};

static
struct ExprNode *parse_expression_1(struct Parser *parser, int min_precedence) {
	if (peek_next(parser) == NULL) {
		errx("expected token!");
	}

	struct ExprNode *lhs = NULL;

	if (peek_next(parser)->type == PUNCTUATION) {
		switch (peek_next(parser)->value) {
			case '(': {
				chop_next(parser); // remove (
				lhs = parse_expression_1(parser, MIN_PRECEDENCE);
				chop_next(parser); // remove )
				break;
			}

			case INC: case DEC:
			case '+': case '-':
			case '~': case '!':
			case '*': case '&': {
				struct ExprNode operator = { chop_next(parser), PRE_UNARY_OP, NULL, NULL };
				operator.rhs = parse_expression_1(parser, precedence[PRE_UNARY_OP]);

				lhs = store_object(parser->allocator, &operator, sizeof operator);
				break;
			}

			default:
				errx("unexpected token type (%c)!", peek_next(parser)->value);
				break;
		}
	}

	else if (peek_next(parser)->type == KEYWORD_SIZEOF) {
		struct ExprNode operator = { chop_next(parser), PRE_UNARY_OP, NULL, NULL };
		operator.rhs = parse_expression_1(parser, precedence[PRE_UNARY_OP]);

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	else {
		struct ExprNode term = { chop_next(parser), TERM, NULL, NULL };
		lhs = store_object(parser->allocator, &term, sizeof term);
	}

	while (peek_next(parser) != NULL && precedence[peek_next(parser)->value] > min_precedence) {
		struct Token *op = chop_next(parser);
		struct ExprNode operator = { op, BINARY_OP, lhs, NULL };

		if (op->value == INC || op->value == DEC) {
			operator.type = POST_UNARY_OP;
		} else {
			operator.rhs = parse_expression_1(parser, precedence[op->value]);
		}

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	return lhs;
}


struct ExprNode *parse_expression(struct Parser *parser) {
	return parse_expression_1(parser, MIN_PRECEDENCE);
}
