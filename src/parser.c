#include "parser.h"

#include "allocator.h"
#include "ast.h"
#include "tokens.h"
#include "util.h"

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

void parser_error(struct Parser *parser, struct Token *, const char *fmt, ...) PRINTF(3,4);
const char *print_token(struct Token *);

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
		parser_error(parser, NULL, "expected `%c`, got %s", c, print_token(tok));
	}
}

static inline
unsigned char expect_next_either(struct Parser *parser, unsigned char a, unsigned char b) {
	struct Token *tok = peek_next(parser);

	if (tok != NULL && tok->type == PUNCTUATION) {
		if (tok->value == a || tok->value == b) {
			chop_next(parser);
			return tok->value;
		}
	}

	parser_error(parser, NULL, "expected `%c` or `%c`, got %s", a, b, print_token(tok));
	return 0;
}

// precedence rules for expressions
#define MIN_PRECEDENCE -1

static
int precedence[] = {
	// closing parens always end sub-expressions
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

#define PREC_UNARY_OP 13

	['.'] = 14, // member access
	['('] = 14, // function call
	['['] = 14, // array subscripting
	[INC] = 14, [DEC] = 14,
};

// Other AST_ExpressionType masks
#define POST_UNARY_OP  0x040
#define LEFT_PAREN     0x080
#define RIGHT_PAREN    0x100
#define SQUARE_PAREN   0x200
#define TYPE           0x400

#define TERM       (LITERAL | STRING | IDENTIFIER)
#define EXPRESSION (TERM | LEFT_PAREN | UNARY_OP)
#define CONTINUE   (POST_UNARY_OP | BINARY_OP)


static
enum AST_ExpressionType get_token_type(struct Token *token) {
	if (token == NULL) return 0;

	switch (token->type) {
		case INT_LITERAL:
		case KEYWORD_FALSE:
		case KEYWORD_TRUE:
			return LITERAL;

		case STRING_LITERAL:
			return STRING;

		case SYMBOL:
			return IDENTIFIER;

		// keyword operators
		case KEYWORD_SIZEOF: return UNARY_OP;
		case KEYWORD_ELSE:   return BINARY_OP;

		case KEYWORD_VOID: case KEYWORD_BOOL: case KEYWORD_CHAR:
		case KEYWORD_INT:  case KEYWORD_U32:
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
				return UNARY_OP;

			case INC: case DEC:
				return UNARY_OP | POST_UNARY_OP;

			case '+': case '-':
			case '*': case SHL:
				return UNARY_OP | BINARY_OP;

			default:
				return BINARY_OP;
		}

		default: return 0;
	}
}


static
struct ExpressionType parse_type(struct Parser *parser) {
	struct Token *basic_type = chop_next(parser);
	struct ExpressionType type = {0};

	switch (basic_type->type) {
		case KEYWORD_VOID: type.type = VOID; break;
		case KEYWORD_BOOL: type.type = BOOL; break;
		case KEYWORD_CHAR: type.type = CHAR; break;
		case KEYWORD_INT:  type.type = INT;  break;
		case KEYWORD_U32:  type.type = U32;  break;
		default: assert(0 && "unreachabel");
	}

	while (peek_next(parser) && peek_next(parser)->type == PUNCTUATION
	                         && peek_next(parser)->value == '*') {
		chop_next(parser);
		type.pointers++;
	}

	return type;
}


static
int sizeof_type(struct ExpressionType type) {
	if (type.pointers > 0) return 4;

	switch (type.type) {
		case VOID: return 0;
		case BOOL: return 1;
		case CHAR: return 1;
		case INT:  return 4;
		case U32:  return 4;

		case UNKNOWN:
			assert(0 && "unreachable");
	}
}


static
struct AST_Expression *parse_expression_1(struct Parser *parser, int min_precedence) {
	struct AST_Expression *lhs = NULL;
	int type = get_token_type(peek_next(parser)) & EXPRESSION;

	switch (type) {
		case LEFT_PAREN: {
			chop_next(parser); // remove (

			// check if type cast
			if (get_token_type(peek_next(parser)) == TYPE) {
				struct Token *reference = parser->tokens;
				struct ExpressionType type = parse_type(parser);
				expect_next(parser, ')');

				struct AST_Expression type_cast = {
					.type = TYPE_CAST,
					.type_cast = {
						.token = reference,
						.type  = type,
						.rhs   = parse_expression_1(parser, PREC_UNARY_OP),
					}
				};

				lhs = store_object(parser->allocator, &type_cast, sizeof type_cast);
			}

			else {
				lhs = parse_expression_1(parser, MIN_PRECEDENCE);
				expect_next(parser, ')');
			}

			break;
		}

		case UNARY_OP: {
			struct Token *operator = chop_next(parser);

			struct AST_Expression op = {
				.type = UNARY_OP,
				.unary_op = {
					.token = operator,
					.rhs = NULL,
				}
			};

			// special sizeof rules:
			// argument can be (type), cannot be a type cast
			if (operator->type == KEYWORD_SIZEOF) {
				if (get_token_type(peek_next(parser)) & LEFT_PAREN &&
				    get_token_type(peek_next2(parser)) == TYPE) {
					expect_next(parser, '(');
					struct ExpressionType T = parse_type(parser);
					expect_next(parser, ')');

					// evaluate sizeof (type) here
					struct AST_Expression static_sizeof = {
						.type = LITERAL,
						.literal = {
							.token = op.unary_op.token,
							.type = T,
							.value = sizeof_type(T),
						},
					};

					lhs = store_object(parser->allocator, &static_sizeof, sizeof static_sizeof);
					break; // success
				}

				else if (get_token_type(peek_next(parser)) == TYPE) {
					parser_error(parser, NULL, "expected parentheses around type name in sizeof expression");
				}
			}

			op.unary_op.rhs = parse_expression_1(parser, PREC_UNARY_OP);
			lhs = store_object(parser->allocator, &op, sizeof op);
			break;
		}

		case LITERAL: {
			struct Token *token = chop_next(parser);
			struct ExpressionType T = {
				.type = (token->value > INT_MAX) ? U32 : INT,
				.pointers = 0
			};

			struct AST_Expression literal = {
				.type = LITERAL,
				.literal = { token, T, token->value },
			};

			lhs = store_object(parser->allocator, &literal, sizeof literal);
			break;
		}

		case STRING: {
			struct AST_Expression string = {
				.type = STRING,
				.string = { chop_next(parser) },
			};

			lhs = store_object(parser->allocator, &string, sizeof string);
			break;
		}

		case IDENTIFIER: {
			struct AST_Expression identifier = {
				.type = IDENTIFIER,
				.identifier = { .token = chop_next(parser) },
			};

			lhs = store_object(parser->allocator, &identifier, sizeof identifier);
			break;
		}

		default: {
			parser_error(parser, NULL, "expected expression, got %s", print_token(peek_next(parser)));
			break;
		}
	}

	// continue parsing binary operators / postfix unary operators

	while ((get_token_type(peek_next(parser)) & CONTINUE)
		&& precedence[peek_next(parser)->value] > min_precedence) {

		struct Token *op = chop_next(parser);
		enum AST_ExpressionType type = get_token_type(op) & CONTINUE;
		struct AST_Expression operator = { .type = type };

		if (type == POST_UNARY_OP) {
			op->value += 1; // convert operator to post-fix
			operator.unary_op.token = op;
			operator.unary_op.rhs = parse_expression_1(parser, MIN_PRECEDENCE);
		}

		else {
			bool func_call = op->value == '(';
			bool array_sub = op->value == '[';

			if (array_sub) puts("here!");

			int prec = precedence[op->value];
			if (func_call || array_sub) prec = MIN_PRECEDENCE;

			operator.binary_op.token = op;
			operator.binary_op.lhs = lhs;
			operator.binary_op.rhs = parse_expression_1(parser, prec);

			if (func_call) expect_next(parser, ')');
			if (array_sub) expect_next(parser, ']');
		}

		lhs = store_object(parser->allocator, &operator, sizeof operator);
	}

	return lhs;
}


struct AST_Expression *parse_expression(struct Parser *parser) {
	return parse_expression_1(parser, MIN_PRECEDENCE);
}


// struct DeclNode *parse_declaration(struct Parser *parser) {
// 	struct DeclNode decl = {0};

// 	if (token_typeof(peek_next(parser)) != TYPE) {
// 		parser_err(parser, "expected type as start of declaration");
// 		return NULL;
// 	}

// 	decl.type = parse_type(parser);
// 	decl.id = chop_next(parser);

// 	if (expect_next2(parser, '=', COM) == COM) {
// 		decl.constant = true;
// 	}

// 	struct Parser reference = *parser;
// 	decl.expr = parse_expression(parser);

// 	if (decl.constant && !parser->errors) {
// 		if (!fold_expression(decl.expr)) {
// 			parser_err(&reference, "failed to fold constant expression");
// 		}
// 	}

// 	return store_object(parser->allocator, &decl, sizeof decl);
// }

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


const char *print_token(struct Token *token) {
	switch (token->type) {
		case INT_LITERAL:    return "integer constant";
		//case FLOAT_LITERAL:  return "float constant";
		case STRING_LITERAL: return "string constant";
		case SYMBOL:  return "identifier";

		case KEYWORD + 1 ... KEYWORD_END - 1:
			return "keyword";

		case PREPROC + 1 ... PREPROC_END - 1:
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

		default:
			assert(0 && "unreachable");
	}

	assert(0 && "unreachable");
	return NULL;
}


void parser_error(struct Parser *parser, struct Token *token, const char *fmt, ...) {
	if (token == NULL) token = parser->tokens;
	printf(WHITE "%s:%d:%d: " RED "error: " RESET, token->filename, token->line, token->col);

	va_list args;
	va_start(args, fmt);

	vprintf(fmt, args);
	va_end(args);

	putchar('\n');
	parser->errors++;
}
