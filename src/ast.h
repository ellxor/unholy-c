#ifndef AST_H
#define AST_H

// type info for AST_ExprNode
enum BasicType {
	VOID, BOOL, CHAR, INT, U32, STR,
};

struct ExpressionType {
	enum BasicType type;
	unsigned pointers: 31, temporary: 1;
};
//

enum AST_ExpressionType {
	LITERAL    = 0x01,
	STRING     = 0x02,
	IDENTIFIER = 0x04,
	UNARY_OP   = 0x08,
	BINARY_OP  = 0x10,
	TYPE_CAST  = 0x20,
	FUNC_CALL  = 0x40,
};

struct AST_ExprLiteral {
	struct Token *token;
	struct ExpressionType type;
	int value;
};

struct AST_ExprString {
	struct Token *token;
};

struct AST_ExprIdentifier {
	struct Token *token;
	struct ExpressionType type;
};

struct AST_ExprUnaryOp {
	struct Token *token;
	struct AST_Expression *rhs;
	struct ExpressionType type;
};

struct AST_ExprBinaryOp {
	struct Token *token;
	struct AST_Expression *lhs;
	struct AST_Expression *rhs;
	struct ExpressionType type;
};

struct AST_ExprTypeCast {
	struct Token *token;
	struct ExpressionType type;
	struct AST_Expression *rhs;
};

struct AST_ExprFuncCall {
	struct Token *token;
	struct ExpressionType type;
	struct AST_Expression *func;
	struct AST_Expression *args;
};

struct AST_Expression {
	enum AST_ExpressionType type;

	union {
		struct AST_ExprLiteral    literal;
		struct AST_ExprString     string;
		struct AST_ExprIdentifier identifier;
		struct AST_ExprUnaryOp    unary_op;
		struct AST_ExprBinaryOp   binary_op;
		struct AST_ExprTypeCast   type_cast;
		struct AST_ExprFuncCall   func_call;
	};
};

#endif //AST_H
