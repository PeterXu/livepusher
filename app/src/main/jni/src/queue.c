#include <stdio.h>
#include <malloc.h>

typedef struct queue_node {
	struct queue_node* prev;
	struct queue_node* next;
	void *p;
} node;

static node *s_phead = NULL;
static int s_count = 0;

static node* create_node(void *pval) {
	node *pnode = NULL;
	pnode = (node *) malloc(sizeof(node));
	if (pnode) {
		pnode->prev = pnode->next = pnode;
		pnode->p = pval;
	}
	return pnode;
}

int create_queue() {
	s_phead = create_node(NULL);
	if (!s_phead) {
		return -1;
	}

	s_count = 0;
	return 0;
}

int queue_size() {
	return s_count;
}

static node* get_node(int index) {
	if (index < 0 || index >= s_count) {
		return NULL;
	}
	if (index <= (s_count / 2)) {
		int i = 0;
		node *pnode = s_phead->next;
		while ((i++) < index)
			pnode = pnode->next;
		return pnode;
	}
	int j = 0;
	int rindex = s_count - index - 1;
	node *rnode = s_phead->prev;
	while ((j++) < rindex)
		rnode = rnode->prev;
	return rnode;
}

static node* get_first_node() {
	return get_node(0);
}

static node* get_last_node() {
	return get_node(s_count - 1);
}
void* queue_get(int index) {
	node *pindex = get_node(index);
	if (!pindex) {
		return NULL;
	}
	return pindex->p;
}


void* queue_get_first() {
	return queue_get(0);
}

void* queue_get_last() {
	return queue_get(s_count - 1);
}

int queue_insert_first(void *pval) {
	node *pnode = create_node(pval);
	if (!pnode)
		return -1;
	pnode->prev = s_phead;
	pnode->next = s_phead->next;
	s_phead->next->prev = pnode;
	s_phead->next = pnode;
	s_count++;
	return 0;
}

int queue_append_last(void *pval) {
	node *pnode = create_node(pval);
	if (!pnode)
		return -1;
	pnode->next = s_phead;
	pnode->prev = s_phead->prev;
	s_phead->prev->next = pnode;
	s_phead->prev = pnode;
	s_count++;
	return 0;
}

static int queue_delete(int index) {
	node *pindex = get_node(index);
	if (!pindex) {
		return -1;
	}
	pindex->next->prev = pindex->prev;
	pindex->prev->next = pindex->next;
	free(pindex);
	s_count--;
	return 0;
}

int queue_delete_first() {
	return queue_delete(0);
}

int queue_delete_last() {
	return queue_delete(s_count - 1);
}

int destroy_queue() {
	if (!s_phead) {
		return -1;
	}

	node *pnode = s_phead->next;
	node *ptmp = NULL;
	while (pnode != s_phead) {
		ptmp = pnode;
		pnode = pnode->next;
		free(ptmp);
	}
	free(s_phead);
	s_phead = NULL;
	s_count = 0;
	return 0;
}

