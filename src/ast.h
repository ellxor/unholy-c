#ifndef AST_H_
#define AST_H_

#include "tokens.h"
#include <stdbool.h>

enum ExprNodeType {
	TERM          = 0x1,
	PRE_UNARY_OP  = 0x2,
	POST_UNARY_OP = 0x4,
	BINARY_OP     = 0x8,
};

struct ExprNode {
	struct Token *token;
	enum ExprNodeType type;

	// child nodes
	struct ExprNode *lhs, *rhs;
};

#endif //AST_H_
