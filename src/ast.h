#ifndef AST_H_
#define AST_H_

#include "tokens.h"
#include <stdbool.h>

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

#endif //AST_H_
