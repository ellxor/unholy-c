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
struct Token *peek_next2(struct Parser *parser) {
	return parser->length > 1 ? parser->tokens + 1 : NULL;
}

static inline
struct Token *chop_next(struct Parser *parser) {
	return parser->length--, parser->tokens++;
}

#define MIN_PRECEDENCE -1

static
int precedence[] = {
	[')'] = MIN_PRECEDENCE,
	[']'] = MIN_PRECEDENCE,

	[','] = 0,

	[OR]  = 3, [AND] = 4,
	['|'] = 5, ['^'] = 6, ['&'] = 7,

	[EQ]  = 8, [NEQ] = 8,
	['>'] = 9, ['<'] = 9, [LEQ] = 9, [GEQ] = 9,

	[SHL] = 10, [SHR] = 10,

	['+'] = 11, ['-'] = 11,
	['*'] = 12, ['/'] = 12, ['%'] = 12,

	[PRE_UNARY_OP] = 13,
	[POST_UNARY_OP] = 14,

	['.'] = 14, // member access
	['('] = 14, // function call
	['['] = 14, // array subscripting
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

		case KEYWORD_BOOL:
		case KEYWORD_CHAR:
		case KEYWORD_INT:
		case KEYWORD_I8:
		case KEYWORD_I16:
		case KEYWORD_I32:
		case KEYWORD_U8:
		case KEYWORD_U16:
		case KEYWORD_U32:
		case KEYWORD_VOID:
			return TYPE;

		case PUNCTUATION: switch (token->value) {
			// invalid operators
			case '{': case '}': case ';': return 0;

			// left paren is also function call operator
			case '(': return LEFT_PAREN | BINARY_OP;
			case ')': return RIGHT_PAREN;

			// array subscripting
			case '[': return BINARY_OP;
			case ']': return SQUARE_PAREN;

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
struct ExprNode *parse_type(struct Parser *parser) {
	struct Token *raw_type = chop_next(parser);
	raw_type->pointers = 0;

	while (peek_next(parser) && peek_next(parser)->type == PUNCTUATION
	                         && peek_next(parser)->value == '*') {
		chop_next(parser);
		raw_type->pointers++;
	}

	struct ExprNode type = { raw_type, TYPE, NULL, NULL };
	return store_object(parser->allocator, &type, sizeof type);
}


static
struct ExprNode *parse_expression_1(struct Parser *parser, int min_precedence) {
	if (peek_next(parser) == NULL) {
		errx("expected token!");
	}

	struct ExprNode *lhs = NULL;
	enum ExprNodeType type = token_typeof(peek_next(parser));

	if (type & LEFT_PAREN) {
		chop_next(parser); // remove (

		// check if type cast
		if (token_typeof(peek_next(parser)) == TYPE) {
			lhs = parse_type(parser);

			if (token_typeof(peek_next(parser)) != RIGHT_PAREN) {
				errx("expected )");
			}

			chop_next(parser); // remove )
			lhs->rhs = parse_expression_1(parser, precedence[PRE_UNARY_OP]);
		}

		// normal bracketed expression
		else {
			lhs = parse_expression_1(parser, MIN_PRECEDENCE);

			if (token_typeof(peek_next(parser)) != RIGHT_PAREN) {
				errx("expected )");
			}

			chop_next(parser); // remove )
		}
	}

	else if (type & PRE_UNARY_OP) {
		struct ExprNode operator = { chop_next(parser), PRE_UNARY_OP, NULL, NULL };

		// special `sizeof (type)` case:
		// note: argument to sizeof cannot be type cast
		if (token_typeof(peek_next(parser)) & LEFT_PAREN &&
		    token_typeof(peek_next2(parser)) == TYPE) {

			chop_next(parser); // remove (
			struct ExprNode *type = parse_type(parser);
			chop_next(parser); // remove )

			operator.rhs = type;
		}

		else {
			operator.rhs = parse_expression_1(parser, precedence[PRE_UNARY_OP]);
		}

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	else if (type == TERM) {
		struct ExprNode term = { chop_next(parser), TERM, NULL, NULL };
		lhs = store_object(parser->allocator, &term, sizeof term);
	}

	else {
		errx("expected expression! (got %d)", type);
	}

	// continue parsing binary operators / postfix unary operators

	while ((token_typeof(peek_next(parser)) & (BINARY_OP | POST_UNARY_OP))
		&& precedence[peek_next(parser)->value] > min_precedence) {

		struct Token *op = chop_next(parser);
		struct ExprNode operator = { op, token_typeof(op), lhs, NULL };
		operator.type &= BINARY_OP | POST_UNARY_OP;

		if (operator.type == BINARY_OP) {
			int prec = (op->value == '(' || op->value == '[')
			         ? MIN_PRECEDENCE
				 : precedence[op->value];

			operator.rhs = parse_expression_1(parser, prec);

			// function call
			if (operator.token->value == '(') {
				if (token_typeof(peek_next(parser)) != RIGHT_PAREN) {
					errx("expected )");
				}

				chop_next(parser); // remove )
			}

			// array subscripting
			if (operator.token->value == '[') {
				if (token_typeof(peek_next(parser)) != SQUARE_PAREN) {
					errx("expected ]");
				}

				chop_next(parser); // remove ]
			}
		}

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	return lhs;
}


struct ExprNode *parse_expression(struct Parser *parser) {
	return parse_expression_1(parser, MIN_PRECEDENCE);
}
