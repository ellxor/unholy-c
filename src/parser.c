#include "parser.h"

#include "tokens.h"
#include "util/allocator.h"
#include "util/error.h"
#include "util/util.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

// if offset is NULL, use current token instead
void parser_err(struct Parser *, const char *fmt, ...) PRINTF(2,3);
void log_error(struct Token *, const char *fmt, ...) PRINTF(2,3);

// pretty print token/type
static const char *print_token(struct Token *);
static const char *print_type(enum ExprNodeType);

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
void expect_next(struct Parser *parser, unsigned char c) {
	struct Token *tok = peek_next(parser);

	if (tok != NULL && tok->type == PUNCTUATION && tok->value == c) {
		chop_next(parser);
	} else {
		parser_err(parser, "expected `%c`, got %s", c, print_token(tok));
	}
}

static inline
unsigned char expect_next2(struct Parser *parser, unsigned char a, unsigned char b) {
	struct Token *tok = peek_next(parser);

	if (tok != NULL && tok->type == PUNCTUATION) {
		if (tok->value == a || tok->value == b) {
			chop_next(parser);
			return tok->value;
		}
	}

	parser_err(parser, "expected `%c` or `%c`, got %s", a, b, print_token(tok));
	return 0;
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
	[INC] = 14, [DEC] = 14,
};

#define PREC_PRE_UNARY_OP  13
#define PREC_POST_UNARY_OP 14

static
enum ExprNodeType token_typeof(struct Token *token) {
	if (token == NULL) return 0;

	switch (token->type) {
		case INT: case FLOAT: case STRING:
		case KEYWORD_FALSE: case KEYWORD_TRUE:
			return LITERAL;

		case IDENT:
			return IDENTIFIER;

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
			case '*': case SHL:
				return PRE_UNARY_OP | BINARY_OP;

			default:
				return BINARY_OP;
		}

		case TOK_EOF: return END_OF_FILE;
		default: return 0;
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

#define TERM (LITERAL | IDENTIFIER)
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
				expect_next(parser, ')');
				lhs->rhs = parse_expression_1(parser, PREC_PRE_UNARY_OP);
			}

			else {
				lhs = parse_expression_1(parser, MIN_PRECEDENCE);
				expect_next(parser, ')');
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

					expect_next(parser, '(');
					op.rhs = parse_type(parser);
					expect_next(parser, ')');
					lhs = store_object(parser->allocator, &op, sizeof op);
					break; // success
				}

				else if (token_typeof(peek_next(parser)) == TYPE) {
					parser_err(parser, "expected parentheses around type name in sizeof expression");
				}
			}

			op.rhs = parse_expression_1(parser, PREC_PRE_UNARY_OP);
			lhs = store_object(parser->allocator, &op, sizeof op);
			break;
		}

		case LITERAL:
		case IDENTIFIER: {
			struct ExprNode term = { chop_next(parser), type, NULL, NULL };
			lhs = store_object(parser->allocator, &term, sizeof term);
			break;
		}

		default: {
			parser_err(parser, "expected expression, got %s", print_token(peek_next(parser)));
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

			if (func_call) expect_next(parser, ')');
			if (array_sub) expect_next(parser, ']');
		}

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	return lhs;
}


struct ExprNode *parse_expression(struct Parser *parser) {
	return parse_expression_1(parser, MIN_PRECEDENCE);
}

static
bool fold_expression(struct ExprNode *root) {
	switch (root->type) {
		case LITERAL:
			break;

		case PRE_UNARY_OP:
			assert(root->token->type == PUNCTUATION ||
			       root->token->type == KEYWORD_SIZEOF);

			if (root->token->type == KEYWORD_SIZEOF) {
				// TODO: add type checking of expressions
				errx("sizeof operator is not implemented!");
			}

			if (!fold_expression(root->rhs)) {
				return false;
			}

			root->type = LITERAL;

			switch (root->token->value) {
				case INC: case DEC:
				case '*': case SHL: return false;
				case '!': root->token->value = !root->rhs->token->value; break;
				case '~': root->token->value = ~root->rhs->token->value; break;
				case '+': root->token->value = +root->rhs->token->value; break;
				case '-': root->token->value = -root->rhs->token->value; break;
				default:
					assert(0 && "unreachable");
					return false;
			}
			break;

		case BINARY_OP:
			assert(root->token->type == PUNCTUATION ||
			       root->token->type == KEYWORD_ELSE);

			if (!fold_expression(root->lhs)) { return false; }
			if (!fold_expression(root->rhs)) { return false; }

			int A = root->lhs->token->value;
			int B = root->rhs->token->value;
			root->type = LITERAL;

			if (root->token->type == KEYWORD_ELSE) {
				root->token->value = A ? A : B;
			}

			else switch (root->token->value) {
				case ',': root->token->value = B; break;

				case OR:  root->token->value = A || B; break;
				case AND: root->token->value = A && B; break;

				case '|': root->token->value = A | B; break;
				case '^': root->token->value = A ^ B; break;
				case '&': root->token->value = A & B; break;

				case EQ:  root->token->value = A == B; break;
				case NEQ: root->token->value = A != B; break;
				case '<': root->token->value = A <  B; break;
				case LEQ: root->token->value = A <= B; break;
				case '>': root->token->value = A >  B; break;
				case GEQ: root->token->value = A >= B; break;

				case SHL: root->token->value = A << B; break;
				case SHR: root->token->value = A >> B; break;

				case '+': root->token->value = A + B; break;
				case '-': root->token->value = A - B; break;
				case '*': root->token->value = A * B; break;
				case '/': root->token->value = A / B; break;
				case '%': root->token->value = A % B; break;

				default:
					assert(0 && "unreachable");
					return false;
			}

			break;

		case TYPE:
			errx("type casts are not supported yet");
			break;

		case POST_UNARY_OP:
			return false;

		default:
			assert(0 && "unreachable");
			return false;
	}

	return true;
}


struct DeclNode *parse_declaration(struct Parser *parser) {
	struct DeclNode decl = {0};

	if (token_typeof(peek_next(parser)) != TYPE) {
		parser_err(parser, "expected type as start of declaration");
		return NULL;
	}

	decl.type = parse_type(parser);
	decl.id = chop_next(parser);

	if (expect_next2(parser, '=', COM) == COM) {
		decl.constant = true;
	}

	struct Parser reference = *parser;
	decl.expr = parse_expression(parser);

	if (decl.constant && !parser->errors) {
		if (!fold_expression(decl.expr)) {
			parser_err(&reference, "failed to fold constant expression");
		}
	}

	return store_object(parser->allocator, &decl, sizeof decl);
}


// static
// const char *print_type(enum ExprNodeType type) {
// 	if (type == EXPRESSION)   return "expression";
// 	if (type == TERM)         return "term";

// 	switch (type) {
// 		case LITERAL:       return "literal";
// 		case IDENTIFIER:    return "identifier";
// 		case PRE_UNARY_OP:
// 		case POST_UNARY_OP: return "unary operator";
// 		case BINARY_OP:     return "binary operator";
// 		case TYPE:          return "type";
// 		case LEFT_PAREN:    return "`(`";
// 		case RIGHT_PAREN:   return "`)`";
// 		case SQUARE_PAREN:  return "`]`";
// 		case END_OF_FILE:   return "EOF";
// 	}

// 	assert(0 && "unreachable");
// 	return NULL;
// }

static
const char *print_token(struct Token *token) {
	switch (token->type) {
		case INT:    return "integer constant";
		case FLOAT:  return "float constant";
		case STRING: return "string constant";
		case IDENT:  return "identifier";

		case KEYWORD_BOOL ... KEYWORD_WHILE:
			return "keyword";

		case PREPROC_ASSERT ... PREPROC_WARNING:
			return "preprocessor directive";

		case TOK_EOF: return "EOF";

		case KEYWORD:
		case PREPROC:
		case NONE:
			assert(0 && "unreachable");
			return NULL;

		case PUNCTUATION:
			switch (token->value) {
				case INC:   return "`++`";
				case DEC:   return "`--`";
				case SHL:   return "`<<`";
				case SHR:   return "`>>`";
				case EQ:    return "`==`";
				case NEQ:   return "`!=`";
				case LEQ:   return "`<=`";
				case GEQ:   return "`>=`";
				case AND:   return "`&&`";
				case OR:    return "`||`";
				case COM:   return "`::`";

				default: {
					static char buffer[4] = "` `";
					buffer[1] = token->value;
					return buffer;
				}
			}
	}

	assert(0 && "unreachable");
	return NULL;
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

void log_error(struct Token *token, const char *fmt, ...) {
	printf(WHITE "%s:%d:%d: " RED "error: " RESET, token->filename, token->line, token->col);

	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);
	va_end(args);

	putchar('\n');
}
