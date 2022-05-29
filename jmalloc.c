#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "jmalloc.h"

#define MIN_HEAP_SIZE (2*1024)
#define MALLOC_SIZE_THRESHOLD (2048)
#define SMALL_BLOCKS_MEM_PERCENT 0.60

#define BLOCK_HEADER_SZ (offsetof(block_t, node))
#define FREE_BLOCK_HEADER_SZ (sizeof(block_t))
#define INIT_ID (0x00aa5500)

// a block in heap memory
typedef struct
{
    size_t size;  // add free bit last 3 bits bit[1-2] size type
    int used;     // free or used flag
    ll_t node;    // used if it's free block
} block_t;


enum
{
    TINY = 0,  //512
    NORMAL,    // 16K
    HUGE,      // 1Mb
};

// USE FLAG
typedef enum use_flag
{
    FREE = 0,
    USED
} use_flag_t;


// create two free list
static LIST_INIT(small_free_list);
static LIST_INIT(large_free_list);
// A large memory heap context pointer
static user_heap_info_t *p_local_heap_info = NULL;


static block_t *find_block(user_heap_info_t *heap, block_t *last, size_t size)
{
    block_t *block = NULL;//heap->free_list;

    /*
    while(block)
    {
        if(block->size < size)
        {
            *last = block;
            block = block->next;
        }    
        else
        {
            break;
        }
                
    }*/

    
    return block;
}


// align size to 8 bytes
size_t align8(size_t s)
{

    if(s & 7 == 0)
    {
        return s;
    }

    s = ((s >> 3) + 1) << 3;
    return s;
}



static block_t *block_allocate(user_heap_info_t *heap, size_t size)
{
    block_t *fit = NULL;
    //block_t *free_node = NULL;
    // to compute the total size of a block
    size_t need_allocate_size = BLOCK_HEADER_SZ + size;
    need_allocate_size = align8(need_allocate_size);
    
    //size_t remain = heap->max_size - heap->offset - 1;
    if(need_allocate_size > MALLOC_SIZE_THRESHOLD)  // need much memory
    {
        // get usage of memory over 50% then coalesce
        // get from large block free list
        // FOR EACH to find a fit block
    }
    else
    {
        // the count of free list node over 30 then coalesce
        // get block from small block free list
        // delete old node from free list
        
        //TODO add block split
    }

    // return the best fit block
    return fit;
}



static block_t *block_coalesce(block_t *prev, block_t *cur)
{
    if(prev == NULL)
    {
        return cur;
    }

    if(cur == NULL)
    {
        return prev;
    }
    
    //check is neighbor then coalesce
    if(prev->used == USED && cur->used == USED &&
       ((unsigned char *)prev + BLOCK_HEADER_SZ + prev->size) == (unsigned char *)cur)
    {
        // size add
        prev->size += BLOCK_HEADER_SZ + cur->size;
        // remove cur node from free list
        list_del(&cur->node);

        return prev;
    }
    
    return NULL;
}



// SET block use flag
static void block_set_free(block_t *block, use_flag_t flag)
{
    if(block)
    {
        block->used = flag;
    }
    
}


void *new_malloc(size_t size)
{
    if(p_local_heap_info == NULL || size <= 0)
    {
        return NULL;
    }
    
    block_t *block = block_allocate(p_local_heap_info, size);
    if(block != NULL)
    {
        block_set_free(block, USED);  // block has been allocated
        return (void *)((unsigned char *)block + BLOCK_HEADER_SZ);
    }

    return NULL;
}


void new_free(void *p)
{
    block_t *blk = NULL;
    block_t *free_blk = NULL;
    
    if(p)
    {
        // get block pointer
        blk = container_of(p, block_t, node);
        block_set_free(blk, FREE);

        // if it is small block
        if(blk->size <= MALLOC_SIZE_THRESHOLD)
        {
            list_for_each_entry(free_blk, &small_free_list, node)
            {
                if(free_blk > blk)
                {
                    list_add_(&blk->node, free_blk->node.prev, &free_blk->node);
                    return;
                }
            }

            list_add_tail(&blk->node, &small_free_list);
        }
        else
        {
            list_for_each_entry(free_blk, &large_free_list, node)
            {
                if(free_blk > blk)
                {
                    list_add_(&blk->node, free_blk->node.prev, &free_blk->node);
                    return;
                }
            }

            list_add_tail(&blk->node, &large_free_list);
        }
        
    }
    

    #if 0
    // coalesce  小块超过一定数量开始合并 defered coalesce config setting
    if(block_size > 16kb)
    {
        // set free and dont coalesce
    }
    
    #endif
    
}


//初始化内存区域
// malloc a memory then initialize two free lists
int heap_init(size_t size)
{
    static int id = INIT_ID;
    p_local_heap_info = NULL;
    // require total memory size
    size_t total = sizeof(*p_local_heap_info) + size;
    if(total < MIN_HEAP_SIZE)
    {
        total = MIN_HEAP_SIZE;   
    }
    
    p_local_heap_info = (user_heap_info_t *)sbrk(0);
    if(sbrk(total) == (void *)-1)
    {
        p_local_heap_info = NULL; 
        return -1;
    }
    else
    {
        memset(p_local_heap_info, 0, sizeof(*p_local_heap_info));
        p_local_heap_info->max_size = size;
        p_local_heap_info->hid = (id++);  // set heap id
        
        /*
        split the heap memory region into two blocks 
        suppose that heap info address is aligned
        */
        // the first small block in small free list 
        block_t *small_block = (block_t *)(p_local_heap_info->mem);
        size_t small_mem_total = (size_t)(size * SMALL_BLOCKS_MEM_PERCENT);
        small_block->size = small_mem_total - BLOCK_HEADER_SZ;
        block_set_free(small_block, FREE);
        
        //printf("BLOCK_HEADER_SZ %ld\n", BLOCK_HEADER_SZ);
        printf("small size %zu total %zu\n", small_mem_total, size);
        // add node to link list
        list_add(&small_block->node, &small_free_list);
        
        // init the first block of large free list
        block_t *large_block = (block_t *)(p_local_heap_info->mem + small_mem_total);
        large_block->size = size - small_mem_total - BLOCK_HEADER_SZ;
        block_set_free(large_block, FREE);
        list_add(&large_block->node, &large_free_list);
        
        
    }
    
    
    return 0;
}

