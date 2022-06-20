#ifndef AST_H
#define AST_H

// type info for AST_ExprNode
enum BasicType {
	UNKNOWN, VOID, BOOL, CHAR, INT, U32
};

struct Type {
	enum BasicType type;
	int pointers;
};
//

enum AST_ExprNodeType {
	LITERAL,
	STRING,
	IDENTIFIER,
	UNARY_OP,
	BINARY_OP,
};

struct AST_ExprLiteral {
	struct Type type;
	struct Token *token;
};

struct AST_ExprString {
	struct Token *token;
};

struct AST_ExprIdentifier {
	struct Token *token;
};

struct AST_ExprUnaryOp {
	struct Token *token;
	struct AST_Expression *rhs;
};

struct AST_ExprBinaryOp {
	struct Token *token;
	struct AST_Expression *lhs;
	struct AST_Expression *rhs;
};

struct AST_Expression {
	enum AST_ExprNodeType type;

	union {
		struct AST_ExprLiteral    _0;
		struct AST_ExprString     _1;
		struct AST_ExprIdentifier _2;
		struct AST_ExprUnaryOp    _3;
		struct AST_ExprBinaryOp   _4;
	} node;
};

#endif //AST_H
