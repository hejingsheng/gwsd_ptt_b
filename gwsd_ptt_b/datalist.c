#include "datalist.h"
#include <string.h>

DataList* datalist_create(size_t itemsize) {
	DataList *list = NULL;

	if( itemsize == 0  )
		return NULL;

	list = (DataList*) malloc( sizeof(DataList) );
    if (NULL == list)
    {
        return NULL;
    }
    
	memset(list,0,sizeof(DataList));

	list->itemsize = ((( sizeof(Node) + itemsize ) + 3) / 4) * 4;

	return list;
}


void datalist_finalize(DataList* list) {

	if( list ) {
		datalist_reset( list );
		free(list);
	}
}

size_t datalist_size(DataList* list) {
    if (NULL == list)
    {
        return 0;
    }
    
	return list->count;
}

void datalist_clear(DataList* list) {
	if( list ) {
		datalist_reset( list );
		list->count = 0;
		list->first = NULL;
		list->last = NULL;
	}
}

Node* datalist_front(DataList* list) {
    if (NULL == list)
    {
        return NULL;
    }
    
	return list->first;
}

Node* datalist_last(DataList* list) {
    if (NULL == list)
    {
        return NULL;
    }
    
	return list->last;
}

Node* datalist_next(Node* node) {
	if( node ) {
		return node->next;
	}
	return NULL;
}

Node* datalist_prev(Node* node) {
	if( node ) {
		return node->prev;
	}
	return NULL;
}

static Node* datalist_new_node(DataList* list,const void* data,size_t size) {
	Node* node = NULL;
	
    if (NULL == list)
    {
        return NULL;
    }
    
	if( size > list->itemsize - sizeof(Node) )
	{
		return NULL;
	}

	node = (Node*)malloc( list->itemsize);
	
	if( NULL == node )
	{
		return NULL;
	}
  
	memset(node,0,sizeof(Node));
	node->rptr = (uint8_t*) (node + 1);
	node->wptr = node->rptr;
	node->endptr = ((uint8_t*)node) + list->itemsize;

	if( data && size ) {
		memcpy(node->wptr,data,size);
		node->wptr += size;
	}
	return node;
}

// return inserted node
Node* datalist_push_back(DataList* list,const void* data,size_t size) {
	Node* node = NULL;

    if (NULL == list)
    {
        return NULL;
    }
    
	node = datalist_new_node(list,data,size);
	if( NULL == node )
	{
		return NULL;
	}

	if( list->first == NULL ) {
		list->first = node;
		list->last = node;
	} else {
		list->last->next = node;
		node->prev = list->last;

		list->last = node;
	}
	list->count++;

	return node;
}


//return inserted node
Node* datalist_push_front(DataList* list,const void* data,size_t size) {
	Node* node = NULL;
	node = datalist_new_node(list,data,size);
	if( NULL == node )
		return NULL;

	if( list->first == NULL ) {
		list->first = node;
		list->last = node;
	} else {
		node->next = list->first;
		list->first->prev = node;

		list->first = node;
	}
	list->count++;
	return node;
}

// insert new node before input one,return inserted node
Node* datalist_insert_before(DataList* list,Node* node,const void* data,size_t size) {
	Node* new_node = NULL;
	
	if( list == NULL || node == NULL )
		return NULL;

	new_node = datalist_new_node(list,data,size);
	if( NULL == new_node )
		return NULL;

	if( node == list->first ) {
		list->first->prev = new_node;
		new_node->next = list->first;

		list->first = new_node;
	} else {
		//ASSERT( list->count > 1);
		new_node->prev = node->prev;
		new_node->next = node;

		node->prev->next = new_node;
		node->prev = new_node;
	}

	list->count++;
	return new_node;
}

Node* datalist_insert_after(DataList* list,Node* node,const void* data,size_t size) {
	Node* new_node = NULL;
	
	if( list == NULL || node == NULL )
		return NULL;

	new_node = datalist_new_node(list,data,size);
	if( NULL == new_node )
		return NULL;

	if( node == list->last ) {
		list->last->next = new_node;
		new_node->prev = list->last;

		list->last = new_node;
	} else {
		new_node->prev = node;
		new_node->next = node->next;

		node->next->prev = new_node;
		node->next = new_node;
	}

	list->count++;
	return new_node;
}

// return node before erased or NULL is not exist
Node* datalist_erase(DataList* list,Node* node) {
	Node* pre_node = NULL;
	Node* new_node = NULL;

	//ASSERT( list->count > 0);
    if (NULL == list)
    {
        return NULL;
    }
    
	if( node == list->first ) {
		return datalist_pop_front(list);
	} else if( node == list->last ) {
		return datalist_pop_last(list);
	}

	pre_node = node->prev;
	if( node->prev ) {
		node->prev->next = node->next;
	}
	if( node->next ) {
		node->next->prev = node->prev;
	}

	if( node == list->first ) {
		list->first = node->next;
	}
	if( node == list->last ) {
		list->last = node->prev;
	}

	new_node = node->prev;
	free( node );

	list->count--;
	return new_node;
}

// return new front node or NULL is not exist
Node* datalist_pop_front(DataList* list) {
	Node* first = NULL;
    
    if (NULL == list)
    {
        return NULL;
    }
    
	if( list->first ) {
		first = list->first;
		if( first->next ) {
			first->next->prev = NULL;
			list->first = first->next;
		} else {
			list->first = NULL;
			list->last = NULL;
		}
		free( first );
		list->count--;
	}
	return list->first;
}


// return new last node or NULL is not exist
Node* datalist_pop_last(DataList* list) {
	Node* last = NULL;
    
    if (NULL == list)
    {
        return NULL;
    }
    
	if( list->last ) {
		last = list->last;
		if( last->prev ) {
			last->prev->next = NULL;
			list->last = last->prev;
		} else {
			list->first = NULL;
			list->last = NULL;
		}
		free( last );
		list->count--;
	}

	return list->last;
}

void datalist_reset( DataList *list )
{
	Node *pNodeNext = NULL;
	Node *pNode = datalist_front( list );

	while ( NULL != pNode )
	{
		pNodeNext = pNode->next;      

		free( pNode );

		pNode = pNodeNext; 
	}

}
