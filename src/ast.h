#ifndef AST_H_
#define AST_H_

#include "tokens.h"
#include <stdbool.h>

enum ExprNodeType {
	TERM = 0x01,

	// round parentheses
	LEFT_PAREN  = 0x02,
	RIGHT_PAREN = 0x04,

	// square brackets
	LEFT_BRACKET  = 0x08,
	RIGHT_BRACKET = 0x10,

	// operators
	UNARY_OP  = 0x20,
	BINARY_OP = 0x40,
};

struct ExprNode {
	struct Token *token;
	bool bracketed;

	union {
		// binary operator
		struct { struct ExprNode *lhs, *rhs; };
	};
};

#endif //AST_H_
