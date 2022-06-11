#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "tokens.h"
#include "util/allocator.h"
#include "util/vec.h"

void dump_token(struct Token token) {
	switch (token.type) {
		case INT:		printf("INT(%d)\n", token.value); break;
		case STRING:		printf("STRING(%s)\n", token.text); break;
		case IDENTIFIER:	printf("IDENT(%s)\n", token.text); break;

		case PUNCTUATION:
			if (token.value > 0xff)
				printf("PUNC(%d)\n", token.value);
			else
				printf("PUNC(`%c`)\n", token.value);
			break;

		default:
			printf("unimplemented!\n");
	}
}

void print_tree(struct ExprNode *node) {
	if (node->type == TERM) {
		printf("%d", node->token->value);
		return;
	}

	printf("(");
	print_tree(node->lhs);
	printf(" %c ", node->token->value);
	print_tree(node->rhs);
	printf(")");
}

int main() {
	struct Vec tokens = vec(struct Token);
	struct Allocator allocator = init_allocator();

	lex_file("test.c", &allocator, &tokens);
	puts("\n!!! PARSING DONE !!!\n");

	struct Token *buffer = tokens.mem;
	int count = tokens.length;

	for (int i = 0; i < count; i++) {
		struct Token token = buffer[i];
		dump_token(token);
	}

	struct Parser parser = { buffer, count, &allocator };
	struct ExprNode *root = parse_expression(&parser, -1);

	puts("\nAST DONE !!!\n");
	print_tree(root);
	printf("\n\n");

	vec_free(&tokens);
	free_allocator(&allocator);
}
