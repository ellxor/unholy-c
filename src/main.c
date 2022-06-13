#include <stdio.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "tokens.h"

#include "util/allocator.h"
#include "util/error.h"
#include "util/vec.h"

static
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

static
void print_type(struct ExprNode *node) {
	switch (node->token->type) {
		case KEYWORD_BOOL: printf("bool"); break;
		case KEYWORD_CHAR: printf("char"); break;
		case KEYWORD_INT:  printf("int");  break;
		case KEYWORD_I8:   printf("i8");   break;
		case KEYWORD_I16:  printf("i16");  break;
		case KEYWORD_I32:  printf("i32");  break;
		case KEYWORD_U8:   printf("u8");   break;
		case KEYWORD_U16:  printf("u16");  break;
		case KEYWORD_U32:  printf("u32");  break;
		case KEYWORD_VOID: printf("void"); break;
		default: errx("unreachable");
	}

	if (node->token->pointers) {
		putchar(' ');
	}

	for (int i = 0; i < node->token->pointers; i++) {
		putchar('*');
	}
}

static
void print_tree(struct ExprNode *node) {
	if (!node) {
		printf("NULL");
		return;
	};

	switch (node->type) {
		case TERM:
			if (node->token->type == IDENTIFIER) printf("%s", node->token->text);
			else printf("%d", node->token->value);
			break;

		case BINARY_OP:
			printf("(");
			print_tree(node->lhs);
			if (node->token->type == KEYWORD_ELSE) printf(" else ");
			else printf(" %c ", node->token->value);
			print_tree(node->rhs);
			printf(")");
			break;

		case PRE_UNARY_OP:
			if (node->token->type == KEYWORD_SIZEOF) printf("sizeof(");
			else printf("%c(", node->token->value);
			print_tree(node->rhs);
			printf(")");
			break;

		case POST_UNARY_OP:
			printf("( ");
			print_tree(node->lhs);
			printf(" %c )", node->token->value);
			break;

		case TYPE:
			printf("(cast ");
			print_type(node);
			printf(" ( ");
			print_tree(node->rhs);
			printf(" ))");
			break;

		default:
			errx("unimplemented!");
			break;
	}
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
	struct ExprNode *root = parse_expression(&parser);

	puts("\nAST DONE !!!\n");
	print_tree(root);
	printf("\n\n");

	vec_free(&tokens);
	free_allocator(&allocator);
}
