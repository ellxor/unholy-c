#include <stdio.h>
#include <assert.h>

#include "allocator.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "tokens.h"

void print_expr(struct AST_Expression *expr, int depth) {
	if (expr == NULL) {
		printf("NULL\n");
	}

	for (int i = 0; i < depth; i++) {
		putchar('\t');
	}

	switch (expr->type) {
		case LITERAL:
			printf("%u\n", expr->literal.value);
			break;

		case STRING:
			printf("\"%s\"\n", expr->string.token->text);
			break;

		case IDENTIFIER:
			printf("ID(%s)\n", expr->identifier.token->text);
			break;

		case UNARY_OP:
			assert(0 && "unimplemented!");
			break;

		case BINARY_OP:
			printf("%c\n", expr->binary_op.token->value);
			print_expr(expr->binary_op.lhs, depth + 1);
			print_expr(expr->binary_op.rhs, depth + 1);
			break;

		case TYPE_CAST:
			assert(0 && "unimplemented!");
			break;

		case FUNC_CALL:
			assert(0 && "unimplemented!");
			break;
	}
}

int main() {
	struct Vec tokens = vec(struct Token);
	struct Allocator allocator = init_allocator();

	lex_file("test", &allocator, &tokens);

	struct Token *buffer = tokens.mem;
	int count = tokens.length;

	struct Parser parser = { buffer, count, &allocator, 0 };
	struct AST_Expression *expr = parse_expression(&parser);
	if (parser.errors == 0) print_expr(expr, 0);

	vec_free(&tokens);
	free_allocator(&allocator);
}
