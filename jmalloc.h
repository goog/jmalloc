#ifndef __JMALLOC_H
#define __JMALLOC_H
#include "linked_list.h"



typedef struct user_heap_info
{
    size_t max_size;
    int hid;  // heap id
    //block_t *small_free_list;  // point to block free list
    //struct ll_head *small_free_list;
    //block_t *large_free_list;  // point to block free list
    int used_bytes;    // already used memory space
    char mem[0];       // a large memory region include small blocks and large blocks
} user_heap_info_t;

int heap_init(size_t size);

void *new_malloc(size_t size);
void new_free(void *p);
#endif