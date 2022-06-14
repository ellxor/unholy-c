#include "parser.h"

#include "ast.h"
#include "tokens.h"
#include "util/allocator.h"
#include "util/error.h"
#include "util/util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

// if offset is NULL, use current token instead
void parser_err(struct Parser *, const char *fmt, ...) PRINTF(2,3);

// get type of token
static enum ExprNodeType token_typeof(struct Token *);

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

static inline
void expect_next(struct Parser *parser, enum ExprNodeType type) {
	if (token_typeof(peek_next(parser)) & type) {
		chop_next(parser);
	} else {
		parser_err(parser, "expected ..., got ...");
	}
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

	['.'] = 14, // member access
	['('] = 14, // function call
	['['] = 14, // array subscripting
};

#define PREC_PRE_UNARY_OP  13
#define PREC_POST_UNARY_OP 14

static
enum ExprNodeType token_typeof(struct Token *token) {
	if (token == NULL) return 0;

	switch (token->type) {
		case INT: case FLOAT: case STRING:
		case KEYWORD_FALSE: case KEYWORD_TRUE:
		case IDENTIFIER:
			return TERM;

		// keyword operators
		case KEYWORD_SIZEOF: return PRE_UNARY_OP;
		case KEYWORD_ELSE:   return BINARY_OP;

		case KEYWORD_BOOL: case KEYWORD_CHAR: case KEYWORD_INT:
		case KEYWORD_I8:   case KEYWORD_I16:  case KEYWORD_I32:
		case KEYWORD_U8:   case KEYWORD_U16:  case KEYWORD_U32:
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

#define EXPRESSION (TERM | LEFT_PAREN | PRE_UNARY_OP)
#define CONTINUE (POST_UNARY_OP | BINARY_OP)

static
struct ExprNode *parse_expression_1(struct Parser *parser, int min_precedence) {
	struct ExprNode *lhs = NULL;

	enum ExprNodeType type = token_typeof(peek_next(parser)) & EXPRESSION;

	switch (type) {
		case LEFT_PAREN: {
			chop_next(parser); // remove (

			// check if type cast
			if (token_typeof(peek_next(parser)) == TYPE) {
				lhs = parse_type(parser);
				expect_next(parser, RIGHT_PAREN);
				lhs->rhs = parse_expression_1(parser, PREC_PRE_UNARY_OP);
			}

			else {
				lhs = parse_expression_1(parser, MIN_PRECEDENCE);
				expect_next(parser, RIGHT_PAREN);
			}

			break;
		}

		case PRE_UNARY_OP: {
			struct ExprNode op = { chop_next(parser), PRE_UNARY_OP, NULL, NULL };

			// special sizeof rules:
			// argument can be (type), cannot be a type cast
			if (op.token->type == KEYWORD_SIZEOF) {
				if (token_typeof(peek_next(parser)) & LEFT_PAREN &&
				    token_typeof(peek_next2(parser)) == TYPE) {

					expect_next(parser, LEFT_PAREN);
					struct ExprNode *type = parse_type(parser);
					expect_next(parser, RIGHT_PAREN);
					break; // success
				}

				else if (token_typeof(peek_next(parser)) == TYPE) {
					parser_err(parser, "expected parentheses around type name in sizeof expression");
				}
			}

			else {
				op.rhs = parse_expression_1(parser, PREC_PRE_UNARY_OP);
			}

			lhs = store_object(parser->allocator, &op, sizeof op);
			break;
		}

		case TERM: {
			struct ExprNode term = { chop_next(parser), TERM, NULL, NULL };
			lhs = store_object(parser->allocator, &term, sizeof term);
			break;
		}

		default: {
			parser_err(parser, "expected expression");
			break;
		}
	}

	// continue parsing binary operators / postfix unary operators

	while ((token_typeof(peek_next(parser)) & CONTINUE)
		&& precedence[peek_next(parser)->value] > min_precedence) {

		struct Token *op = chop_next(parser);
		struct ExprNode operator = { op, token_typeof(op) & CONTINUE, lhs, NULL };

		if (operator.type == BINARY_OP) {
			bool func_call = op->value == '(';
			bool array_sub = op->value == '[';

			int prec = precedence[op->value];
			if (func_call || array_sub) prec = MIN_PRECEDENCE;

			operator.rhs = parse_expression_1(parser, prec);

			if (func_call) expect_next(parser, RIGHT_PAREN);
			if (array_sub) expect_next(parser, SQUARE_PAREN);

		}

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	return lhs;
}


struct ExprNode *parse_expression(struct Parser *parser) {
	return parse_expression_1(parser, MIN_PRECEDENCE);
}


void parser_err(struct Parser *parser, const char *fmt, ...) {
	struct Token *token = parser->tokens;

	printf(WHITE "%s:%d:%d: " RED "error: " RESET, token->filename, token->line, token->col);
	parser->errors = true;

	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);
	va_end(args);

	putchar('\n');
}
