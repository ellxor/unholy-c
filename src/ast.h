#ifndef AST_H_
#define AST_H_

#include "tokens.h"
#include <stdbool.h>

enum ExprNodeType {
	TERM = 0x1,

	// round parentheses
	LEFT_PAREN  = 0x2,
	RIGHT_PAREN = 0x4,

	// square brackets
	LEFT_BRACKET  = 0x08,
	RIGHT_BRACKET = 0x10,

	// operators
	PRE_UNARY_OP  = 0x20,
	POST_UNARY_OP = 0x40,
	BINARY_OP     = 0x80,

	END = 0x100,
};

struct ExprNode {
	struct Token *token;
	enum ExprNodeType type;

	// child nodes
	struct ExprNode *lhs, *rhs;
};

#endif //AST_H_
