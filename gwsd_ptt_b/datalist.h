#ifndef _DATA_LIST_H_
#define _DATA_LIST_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _Node {
	struct _Node*	prev;
	struct _Node*	next;

	uint8_t*				rptr;
	uint8_t*				wptr;
	uint8_t*				endptr;
} Node;


typedef struct {
	size_t			itemsize;

	size_t			count;
	Node*		first;
	Node*		last;
} DataList;


DataList* datalist_create(size_t itemsize);
void datalist_finalize(DataList* list);
void datalist_clear(DataList* list);
void datalist_reset(DataList *list);

size_t datalist_size(DataList* list);

Node* datalist_front(DataList* list);
Node* datalist_last(DataList* list);

Node* datalist_next(Node* node);
Node* datalist_prev(Node* node);

// return inserted node
Node* datalist_push_back(DataList* list,const void* data,size_t size);

//return inserted node
Node* datalist_push_front(DataList* list,const void* data,size_t size);

// insert new node before input one,return inserted node
Node* datalist_insert_before(DataList* list,Node* node,const void* data,size_t size);

// insert new node after input one,return inserted node
Node* datalist_insert_after(DataList* list,Node* node,const void* data,size_t size);

// return node before erased or NULL is not exist
Node* datalist_erase(DataList* list,Node* node);

// return new front node or NULL is not exist
Node* datalist_pop_front(DataList* list);

// return new last node or NULL is not exist
Node* datalist_pop_last(DataList* list);


#ifdef __cplusplus
}
#endif

#endif
