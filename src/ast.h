#ifndef AST_H_
#define AST_H_

#include "tokens.h"
#include <stdbool.h>

enum ExprNodeType {
	TERM          = 0x01,
	LEFT_PAREN    = 0x02,
	RIGHT_PAREN   = 0x04,
	PRE_UNARY_OP  = 0x08,
	POST_UNARY_OP = 0x10,
	BINARY_OP     = 0x20,
};

struct ExprNode {
	struct Token *token;
	enum ExprNodeType type;

	// child nodes
	struct ExprNode *lhs, *rhs;
};

#endif //AST_H_
