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
	// `)` always end expressions
	[')'] = MIN_PRECEDENCE,

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
enum ExprNodeType token_typeof(struct Token *token) {
	if (!token) return 0;

	switch (token->type) {
		case INT:
		case FLOAT:
		case STRING:
		case IDENTIFIER:
			return TERM;

		// keyword operators
		case KEYWORD_SIZEOF: return PRE_UNARY_OP;
		case KEYWORD_ELSE:   return BINARY_OP;

		// types
		case KEYWORD_BOOL:
		case KEYWORD_CHAR:
		case KEYWORD_INT:
		case KEYWORD_I8:
		case KEYWORD_I16:
		case KEYWORD_I32:
		case KEYWORD_U8:
		case KEYWORD_U16:
		case KEYWORD_U32:
			return TYPE;

		case PUNCTUATION: switch (token->value) {
			case '{': case '}':
			case ';':
				return 0;

			case '(': return LEFT_PAREN;
			case ')': return RIGHT_PAREN;

			case '!': case '~':
				return PRE_UNARY_OP;

			case INC: case DEC:
				return PRE_UNARY_OP | POST_UNARY_OP;

			case '+': case '-':
			case '&': case '*':
				return PRE_UNARY_OP | BINARY_OP;

			default:
				return BINARY_OP;
		}

		default:
			return 0;
	}
}

static
struct ExprNode *parse_expression_1(struct Parser *parser, int min_precedence) {
	if (peek_next(parser) == NULL) {
		errx("expected token!");
	}

	struct ExprNode *lhs = NULL;
	enum ExprNodeType type = token_typeof(peek_next(parser));

	if (type == LEFT_PAREN) {
		chop_next(parser); // remove (
		lhs = parse_expression_1(parser, MIN_PRECEDENCE);

		if (token_typeof(peek_next(parser)) != RIGHT_PAREN) {
			errx("expected )");
		}

		chop_next(parser); // remove )
	}

	else if (type & PRE_UNARY_OP) {
		struct ExprNode operator = { chop_next(parser), PRE_UNARY_OP, NULL, NULL };
		operator.rhs = parse_expression_1(parser, precedence[PRE_UNARY_OP]);
		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	else if (type == TERM) {
		struct ExprNode term = { chop_next(parser), TERM, NULL, NULL };
		lhs = store_object(parser->allocator, &term, sizeof term);
	}

	else {
		errx("expected expression!");
	}

	// continue parsing binary operators / postfix unary operators

	while ((token_typeof(peek_next(parser)) & (BINARY_OP | POST_UNARY_OP))
		&& precedence[peek_next(parser)->value] > min_precedence) {

		struct Token *op = chop_next(parser);
		struct ExprNode operator = { op, token_typeof(op), lhs, NULL };
		operator.type &= BINARY_OP | POST_UNARY_OP;

		if (operator.type == BINARY_OP) {
			operator.rhs = parse_expression_1(parser, precedence[op->value]);
		}

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	return lhs;
}


struct ExprNode *parse_expression(struct Parser *parser) {
	return parse_expression_1(parser, MIN_PRECEDENCE);
}
