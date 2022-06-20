#ifndef PARSER_H_
#define PARSER_H_

#include "allocator.h"
#include "tokens.h"
#include "util.h"

#include <stdbool.h>

struct Parser {
	struct Token *tokens;
	int length;

	struct Allocator *allocator;
	bool errors;
};

enum ExprNodeType {
	LITERAL       = 0x001,
	IDENTIFIER    = 0x002,
	PRE_UNARY_OP  = 0x004,
	POST_UNARY_OP = 0x008,
	BINARY_OP     = 0x010,
	TYPE          = 0x020,
	LEFT_PAREN    = 0x040,
	RIGHT_PAREN   = 0x080,
	SQUARE_PAREN  = 0x100,
	END_OF_FILE   = 0x200,
};

struct ExprNode {
	struct Token *token;
	enum ExprNodeType type;

	// child nodes
	struct ExprNode *lhs, *rhs;
};

struct DeclNode {
	struct Token *id;
	struct ExprNode *type;
	struct ExprNode *expr;
	bool constant;
};

struct ExprNode *parse_expression(struct Parser *);
struct DeclNode *parse_declaration(struct Parser *);

#endif //PARSER_H_
