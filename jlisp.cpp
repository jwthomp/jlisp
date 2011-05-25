#include "reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tokenize stuff

// quote
// atom
// eq
// cons
// car
// cdr
// cond

// (
// )
// atom

typedef enum {
	ROOT_NODE_TYPE,
	OPEN_PAREN,
	CLOSED_PAREN,
	ATOM
} lisp_e;

typedef struct node_s {
	int type;
	char value[256];
	struct node_s *next;
} node_t;


typedef struct object_s {
	unsigned long type;
	unsigned long size;
	void *data;
	struct object_s *next;
} object_t;

//
// Defines
//
#define ROOT_NODE node_create(ROOT_NODE_TYPE, NULL)
#define OPEN_PAREN_NODE node_create(OPEN_PAREN, NULL)
#define CLOSED_PAREN_NODE node_create(CLOSED_PAREN, NULL)


//
// Prototypes
//
node_t *tokenize_atom(stream_t *, node_t *);
node_t * parse_statements(node_t *);

void print_nodes(node_t *node)
{
	if (node == NULL) {
		printf("\n");
		return;
	}

	switch (node->type) {
		case ROOT_NODE_TYPE:
			printf("root node\n");
			print_nodes(node->next);
			break;

		case OPEN_PAREN:
			printf("OPEN_PAREN ");
			print_nodes(node->next);
			break;

		case CLOSED_PAREN:
			printf("CLOSED_PAREN ");
			print_nodes(node->next);
			break;
		case ATOM:
			printf("(ATOM %s) ", node->value);
			print_nodes(node->next);
			break;
	}
}

node_t * node_create(lisp_e type, char *value)
{
	node_t *n = (node_t *)malloc(sizeof(node_t));
	n->type = type;
	if (value == NULL) {
		n->value[0] = NULL;
	} else {
		strcpy(n->value, value);
	}
	n->next = NULL;
	return n;
}
	

node_t *tokenize_atom(stream_t *stream, node_t * node_list)
{
	node_t *n = node_create(ATOM, 0);
	int index = 0;
	node_list->next = n;

	char ch = stream->pop();
	while(1) {
		switch (ch) {
			case ' ' :
				stream->restore();
				return n;
			case '(' :
				stream->restore();
				return n;
			case ')' :
				stream->restore();
				return n;
			default: 
				n->value[index++] = ch;
				break;
		}
		ch = stream->pop();
	}

}

node_t *tokenize_root(stream_t *stream, node_t *node_list)
{
	char ch = stream->pop();
	while(ch != '\0') {
		switch (ch) {
			case ' ' :
				break;
			case '(' :
			{
				node_t *n = OPEN_PAREN_NODE;
				node_list->next = n;
				node_list = n;
				break;
			}
			case ')' :
			{
				node_t *n = CLOSED_PAREN_NODE;
				node_list->next = n;
				node_list = n;
				break;
			}
			default:
			{
				stream->restore();
				node_list = tokenize_atom(stream, node_list);
				break;
			}
		}
		ch = stream->pop();
	}
}

node_t * parse_sequence(node_t *);

node_t * parse_expression(node_t *node_list) {
	switch (node_list->type) {
		case OPEN_PAREN:
		{
printf("list\n");
			node_list = parse_sequence(node_list->next);
			if (node_list == NULL) {
				printf("Invalid list\n");
				exit(0);
			}

			if (node_list->type != CLOSED_PAREN) {
				printf("Improperly terminated list\n");
				exit(0);
			}
printf("end list\n");

			return node_list->next;
		}
		case ATOM:
printf("atom: %s\n", node_list->value);
			return node_list->next;
		default:
			printf("Invalid expression: %d\n", node_list->type);
			exit(0);
	}
}

node_t * parse_sequence(node_t *node_list)
{
	node_list = parse_expression(node_list);
	if (node_list == NULL) {
		return NULL;
	}

	if (node_list->type == CLOSED_PAREN) {
		return node_list;
	} else {
		return parse_sequence(node_list);
	}
}

int main2(int argc, char *argv[])
{
	char const *code = "(cons (quote 1) 2)\0";
	stream_t *stream = stream_create(code);

	node_t *root_node = ROOT_NODE;
	tokenize_root(stream, root_node);
	parse_sequence(root_node->next);

	print_nodes(root_node);

	printf("%s\n", stream->code);
}
