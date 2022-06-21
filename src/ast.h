#ifndef AST_H
#define AST_H

// type info for AST_ExprNode
enum BasicType {
	UNKNOWN, VOID, BOOL, CHAR, INT, U32
};

struct ExpressionType {
	enum BasicType type;
	int pointers;
};
//

enum AST_ExpressionType {
	LITERAL    = 0x1,
	STRING     = 0x2,
	IDENTIFIER = 0x4,
	UNARY_OP   = 0x8,
	BINARY_OP  = 0x10,
	TYPE_CAST  = 0x20,
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

struct AST_Expression {
	enum AST_ExpressionType type;

	union {
		struct AST_ExprLiteral    literal;
		struct AST_ExprString     string;
		struct AST_ExprIdentifier identifier;
		struct AST_ExprUnaryOp    unary_op;
		struct AST_ExprBinaryOp   binary_op;
		struct AST_ExprTypeCast   type_cast;
	};
};

#endif //AST_H
