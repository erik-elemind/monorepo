/*
 * mm.c - fast and memory-efficient malloc package
 *
 * In this package, a block is allocated with 8 byte header. Free
 * blocks are maintained by Red-black tree, which allows logarithmic
 * time complexity of best-fit malloc and free. If a request size is larger
 * than any free blocks, it simply increases the brk pointer.
 * When a block is freed, immediate coalescing occurs. Realloc is
 * not implemented here.
 *
 * Red-black tree implementation is modified from
 * - http://web.mit.edu/~emin/www.old/source_code/red_black_tree/red_black_tree.c
 * - http://en.wikipedia.org/wiki/Red%E2%80%93black_tree
 * Red-black tree uses block size as key.
 *
 * Setting DEBUG flag will print core function calls and full RBtree contents.
 * Setting CHECK flag will check heap consistency each time malloc and free are called.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memman.h>
#include <unistd.h>
#include <string.h>


/*
 * Block structure
 *
 * We assume sizeof(size_t) == 4 and sizeof(void*) == 4.
 *
 * An allocated block
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- current
 * |  Size of previous block                                 |0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Size of current block                                  |0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- user
 * .                                                               .
 * .  User data                                                    .
 * .                                                               .
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- next
 * |  Size of current block                                  |0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- (brk)
 *
 * The size of current block is stored in both current block's header
 * and next block's header. So brk is always 4 byte off from 8-byte
 * boundary. Like libc malloc, user is always aligned to 8-byte boundary.
 * Since last 3 bits of size will be 0, the last bit is used for
 * indicating whether the block is free. (1 if free)
 * 
 * An free block
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- current
 * |  Size of previous block                                 |0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Size of current block                                  |0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Address of left child                                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Address of right child                                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Address of parent                                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  Garbage                                                    |R|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * .                                                               .
 * .  Garbage                                                      .
 * .                                                               .
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- next
 * |  Size of current block                                  |0|0|F|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ <- (brk)
 *
 * Addresses of children and parent are added in order to maintain Red-black
 * tree. Color is stored in the last bit after parent address. (1 if red)
 * Therefore, in order to store free block in the tree, the block should
 * be at least 24-byte (MIN_BLOCK_SIZE) including the header. Thus, this
 * package always allocates more than or equal to MIN_BLOCK_SIZE. Note that
 * free block which is smaller than MIN_BLOCK_SIZE can still happen when 
 * a block is assigned from a bigger free block.
 *
 */

#define MM_DEBUG (0U)
#define MM_CHECK (1U)

/* double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define HEADER_SIZE 8
#define MIN_BLOCK_SIZE 24
#define FAIL ((void*)-1)

/*
 * pointer macros
 */
#define PREV_SIZE(p) (*(size_t*)(p))
/* returns block size without free bit. Note that this is r-value */
#define PREV_SIZE_MASKED(p) (PREV_SIZE(p) & ~0x7)
#define PREV_FREE(p) (PREV_SIZE(p) & 0x1)
#define CUR_SIZE(p) (*(size_t*)((p) + 4))
#define CUR_SIZE_MASKED(p) (CUR_SIZE(p) & ~0x7)
#define CUR_FREE(p) (CUR_SIZE(p) & 0x1)
#define RB_LEFT(p) (*(void**)((p) + 8))
#define RB_RIGHT(p) (*(void**)((p) + 12))
#define RB_PARENT(p) (*(void**)((p) + 16))
#define RB_RED(p) (*(int*)((p) + 20))
#define PREV_BLOCK(p, sz) ((p) - (sz))
#define NEXT_BLOCK(p, sz) ((p) + (sz))
#define USER_BLOCK(p) ((p) + HEADER_SIZE)
/* should it be in Red-black tree? */
#define IS_IN_RB(p) (CUR_SIZE_MASKED(p) >= MIN_BLOCK_SIZE)

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(mm_t* mm, void* buf, size_t buf_size)
{
    /* init memory */
    mem_init(&(mm->mem), buf, buf_size);
    /* allocate root and nil nodes */
    if((mm->rb_root = mm->rb_null = mem_sbrk(&(mm->mem), 4 + MIN_BLOCK_SIZE)) == FAIL) return -1;
    /* assign sentinel values */
    RB_LEFT(mm->rb_root) = RB_RIGHT(mm->rb_root) = mm->rb_null;
    RB_RED(mm->rb_root) = 0;
    /* prevent coalesce by setting free bit to 0*/
    PREV_SIZE(NEXT_BLOCK(mm->rb_null, MIN_BLOCK_SIZE)) = 0;
    return 0;
}

/*
 * rb_find - find the smallest free block which is bigger than or equal to size.
 */
static void* rb_find(mm_t* mm, size_t size){
    void *node = RB_LEFT(mm->rb_root), *best = mm->rb_null;
    while(node != mm->rb_null){
        if(CUR_SIZE_MASKED(node) < size){
            node = RB_RIGHT(node);
        }else{
            best = node;
            node = RB_LEFT(node);
        }
    }
    return best;
}


#if ((defined(MM_DEBUG) && (MM_DEBUG > 0U)) || (defined(MM_CHECK) && (MM_CHECK > 0U)))
/*
 * rb_find_exact - check whether block is in Red-black tree or not.
 */
static int rb_find_exact(mm_t* mm, void *block){
    void *node = RB_LEFT(mm->rb_root);
    while(node != mm->rb_null){
        if(node == block){
            return 1;
        }else if(CUR_SIZE_MASKED(node) > CUR_SIZE_MASKED(block)){
            node = RB_LEFT(node);
        }else if(CUR_SIZE_MASKED(node) == CUR_SIZE_MASKED(block)){
            if(node > block){
                node = RB_LEFT(node);
            }else{
                node = RB_RIGHT(node);
            }
        }else{
            node = RB_RIGHT(node);
        }
    }
    return 0;
}
#endif // (defined(MM_DEBUG) && (MM_DEBUG > 0U))


/*
 * rb_successor - find the next node of node in ascending order
 */
static void* rb_successor(mm_t* mm, void *node){
    void *succ, *left;
    if((succ = RB_RIGHT(node)) != mm->rb_null){
    	while((left = RB_LEFT(succ)) != mm->rb_null){
            succ = left;
        }
        return succ;
    }else{
        succ = RB_PARENT(node);
        while(RB_RIGHT(succ) == node){
            node = succ;
            succ = RB_PARENT(succ);
        }
        if(succ == mm->rb_root) return mm->rb_null;
        return succ;
    }
}

/*
 * rb_rotate_left - rotate node and children of node to the left
 */
static void rb_rotate_left(mm_t* mm, void *node){
    void *right;

    right = RB_RIGHT(node);
    RB_RIGHT(node) = RB_LEFT(right);
    if(RB_LEFT(right) != mm->rb_null)
        RB_PARENT(RB_LEFT(right)) = node;

    RB_PARENT(right) = RB_PARENT(node);
    if(node == RB_LEFT(RB_PARENT(node))){
        RB_LEFT(RB_PARENT(node)) = right;
    }else{
        RB_RIGHT(RB_PARENT(node)) = right;
    }
    RB_LEFT(right) = node;
    RB_PARENT(node) = right;
}

/*
 * rb_rotate_right - rotate node and children of node to the right
 */
static void rb_rotate_right(mm_t* mm, void *node){
    void *left;

    left = RB_LEFT(node);
    RB_LEFT(node) = RB_RIGHT(left);
    if(RB_RIGHT(left) != mm->rb_null)
        RB_PARENT(RB_RIGHT(left)) = node;

    RB_PARENT(left) = RB_PARENT(node);
    if(node == RB_LEFT(RB_PARENT(node))){
        RB_LEFT(RB_PARENT(node)) = left;
    }else{
        RB_RIGHT(RB_PARENT(node)) = left;
    }
    RB_RIGHT(left) = node;
    RB_PARENT(node) = left;
}

/*
 * rb_fix - restore properties of Red-black tree after deleting
 */
static void rb_fix(mm_t* mm, void *node){
    void *root, *sib;
    root = RB_LEFT(mm->rb_root);
    while(!RB_RED(node) && node != root){
        if(node == RB_LEFT(RB_PARENT(node))){
            sib = RB_RIGHT(RB_PARENT(node));
            if(RB_RED(sib)){
                RB_RED(sib) = 0;
                RB_RED(RB_PARENT(node)) = 1;
                rb_rotate_left(mm, RB_PARENT(node));
                sib = RB_RIGHT(RB_PARENT(node));
            }
            if(!RB_RED(RB_RIGHT(sib)) && !RB_RED(RB_LEFT(sib))){
                RB_RED(sib) = 1;
                node = RB_PARENT(node);
            }else{
                if(!RB_RED(RB_RIGHT(sib))){
                    RB_RED(RB_LEFT(sib)) = 0;
                    RB_RED(sib) = 1;
                    rb_rotate_right(mm, sib);
                    sib = RB_RIGHT(RB_PARENT(node));
                }
                RB_RED(sib) = RB_RED(RB_PARENT(node));
                RB_RED(RB_PARENT(node)) = 0;
                RB_RED(RB_RIGHT(sib)) = 0;
                rb_rotate_left(mm, RB_PARENT(node));
                node = root;
            }
        }else{
            sib = RB_LEFT(RB_PARENT(node));
            if(RB_RED(sib)){
                RB_RED(sib) = 0;
                RB_RED(RB_PARENT(node)) = 1;
                rb_rotate_right(mm, RB_PARENT(node));
                sib = RB_LEFT(RB_PARENT(node));
            }
            if(!RB_RED(RB_RIGHT(sib)) && !RB_RED(RB_LEFT(sib))){
                RB_RED(sib) = 1;
                node = RB_PARENT(node);
            }else{
                if(!RB_RED(RB_LEFT(sib))){
                    RB_RED(RB_RIGHT(sib)) = 0;
                    RB_RED(sib) = 1;
                    rb_rotate_left(mm, sib);
                    sib = RB_LEFT(RB_PARENT(node));
                }
                RB_RED(sib) = RB_RED(RB_PARENT(node));
                RB_RED(RB_PARENT(node)) = 0;
                RB_RED(RB_LEFT(sib)) = 0;
                rb_rotate_right(mm, RB_PARENT(node));
                node = root;
            }
        }
        
    }
    RB_RED(node) = 0;
}

/*
 * rb_delete - delete node from Red-black tree
 */
static void rb_delete(mm_t* mm, void *node){
    void *m, *c;
    m = RB_LEFT(node) == mm->rb_null || RB_RIGHT(node) == mm->rb_null ? node : rb_successor(mm, node);
    c = RB_LEFT(m) == mm->rb_null ? RB_RIGHT(m) : RB_LEFT(m);
    if((RB_PARENT(c) = RB_PARENT(m)) == mm->rb_root){
        RB_LEFT(mm->rb_root) = c;
    }else{
        if(RB_LEFT(RB_PARENT(m)) == m){
            RB_LEFT(RB_PARENT(m)) = c;
        }else{
            RB_RIGHT(RB_PARENT(m)) = c;
        }
    }
    if(m != node){
        if(!RB_RED(m)) rb_fix(mm, c);
        RB_LEFT(m) = RB_LEFT(node);
        RB_RIGHT(m) = RB_RIGHT(node);
        RB_PARENT(m) = RB_PARENT(node);
        RB_RED(m) = RB_RED(node);
        RB_PARENT(RB_LEFT(node)) = RB_PARENT(RB_RIGHT(node)) = m;
        if(node == RB_LEFT(RB_PARENT(node))){
            RB_LEFT(RB_PARENT(node)) = m;
        }else{
            RB_RIGHT(RB_PARENT(node)) = m;
        }
    }else{
        if(!RB_RED(m)) rb_fix(mm, c);
    }
}

/*
 * rb_insert - insert node into Red-black tree
 */
static void rb_insert(mm_t* mm, void *node){
    void *parent, *child, *sib;

    RB_LEFT(node) = RB_RIGHT(node) = mm->rb_null;
    parent = mm->rb_root;
    child = RB_LEFT(mm->rb_root);
    while(child != mm->rb_null){
        parent = child;
        if(CUR_SIZE_MASKED(child) > CUR_SIZE_MASKED(node)){
            child = RB_LEFT(child);
        }else if(CUR_SIZE_MASKED(child) == CUR_SIZE_MASKED(node)){
            if(child > node){
                child = RB_LEFT(child);
            }else{
                child = RB_RIGHT(child);
            }
        }else{
            child = RB_RIGHT(child);
        }
    }
    RB_PARENT(node) = parent;
    if(parent == mm->rb_root || CUR_SIZE_MASKED(parent) > CUR_SIZE_MASKED(node)){
        RB_LEFT(parent) = node;
    }else if(CUR_SIZE_MASKED(parent) == CUR_SIZE_MASKED(node)){
        if(parent > node){
            RB_LEFT(parent) = node;
        }else{
            RB_RIGHT(parent) = node;
        }
    }else{
        RB_RIGHT(parent) = node;
    }

    RB_RED(node) = 1;
    while(RB_RED(RB_PARENT(node))){
        if(RB_PARENT(node) == RB_LEFT(RB_PARENT(RB_PARENT(node)))){
            sib = RB_RIGHT(RB_PARENT(RB_PARENT(node)));
            if(RB_RED(sib)){
                RB_RED(RB_PARENT(node)) = 0;
                RB_RED(sib) = 0;
                RB_RED(RB_PARENT(RB_PARENT(node))) = 1;
                node = RB_PARENT(RB_PARENT(node));
            }else{
                if(node == RB_RIGHT(RB_PARENT(node))){
                    node = RB_PARENT(node);
                    rb_rotate_left(mm, node);
                }
                RB_RED(RB_PARENT(node)) = 0;
                RB_RED(RB_PARENT(RB_PARENT(node))) = 1;
                rb_rotate_right(mm, RB_PARENT(RB_PARENT(node)));
            }
        }else{
            sib = RB_LEFT(RB_PARENT(RB_PARENT(node)));
            if(RB_RED(sib)){
                RB_RED(RB_PARENT(node)) = 0;
                RB_RED(sib) = 0;
                RB_RED(RB_PARENT(RB_PARENT(node))) = 1;
                node = RB_PARENT(RB_PARENT(node));
            }else{
                if(node == RB_LEFT(RB_PARENT(node))){
                    node = RB_PARENT(node);
                    rb_rotate_right(mm, node);
                }
                RB_RED(RB_PARENT(node)) = 0;
                RB_RED(RB_PARENT(RB_PARENT(node))) = 1;
                rb_rotate_left(mm, RB_PARENT(RB_PARENT(node)));
            }
        }
    }
    RB_RED(RB_LEFT(mm->rb_root)) = 0;
}

#if ((defined(MM_DEBUG) && (MM_DEBUG > 0U)) || (defined(MM_CHECK) && (MM_CHECK > 0U)))

/*
 * rb_print_preorder_impl - recursion implementation of rb_print_preorder
 */
static void rb_print_preorder_impl(mm_t* mm, void *node){
    if(RB_LEFT(node) != mm->rb_null){
        rb_print_preorder_impl(mm, RB_LEFT(node));
    }
    printf("%p : %u\n", node, CUR_SIZE_MASKED(node));
    if(RB_RIGHT(node) != mm->rb_null){
        rb_print_preorder_impl(mm, RB_RIGHT(node));
    }
}

/*
 * rb_print_preorder - print nodes of Red-black tree in preorder
 */
static void rb_print_preorder(mm_t* mm){
    printf("rb_print_preorder() called\n");
    if(RB_LEFT(mm->rb_root) == mm->rb_null){
        printf("empty\n");
    }else{
        rb_print_preorder_impl(mm, RB_LEFT(mm->rb_root));
    }
}

/*
 * rb_check_preorder_impl - recursion implementation of rb_check_preorder
 */
static int rb_check_preorder_impl(mm_t* mm, void *node){
    if(RB_LEFT(node) != mm->rb_null){
        if(!rb_check_preorder_impl(mm, RB_LEFT(node))){
            return 0;
        }
    }
    if(!CUR_FREE(node)){
        printf("%p is in Red-black tree, but is not free block.\n", node);
        return 0;
    }
    if(RB_RIGHT(node) != mm->rb_null){
        if(!rb_check_preorder_impl(mm, RB_RIGHT(node))){
            return 0;
        }
    }
    return 1;
}

/*
 * rb_check_preorder
 *
 * return 0 if there exists allocated block in Red-black tree, 1 otherwise.
 */
static int rb_check_preorder(mm_t* mm){
    if(RB_LEFT(mm->rb_root) != mm->rb_null){
        return rb_check_preorder_impl(mm, RB_LEFT(mm->rb_root));
    }
    return 1;
}


/*
 * mm_check - heap consistency checker. return 0 if something is wrong, 1 otherwise.
 */
int mm_check(mm_t* mm)
{
    void *cur, *end;

    if(!rb_check_preorder(mm)){
        return 0;
    }

    cur = mem_heap_lo(&(mm->mem)) + MIN_BLOCK_SIZE;
    end = mem_heap_hi(&(mm->mem)) - 3;
    while(cur < end){
        if(CUR_FREE(cur)){ // cur is free block
            if(PREV_FREE(cur)){ // contiguous free block
                printf("%p, %p are consecutive, but both are free.\n",
                        PREV_BLOCK(cur, CUR_SIZE_MASKED(cur)), cur);
                return 0;
            }
            if(IS_IN_RB(cur) && !rb_find_exact(mm, cur)){ // cur is not in Red-black tree
                printf("%p is free block, but is not in Red-black tree.\n", cur);
                return 0;
            }
        }else{ // cur is allocated block
        }
        cur = NEXT_BLOCK(cur, CUR_SIZE_MASKED(cur));
    }
    return 1;
}


#endif // (defined(MM_DEBUG) && (MM_DEBUG > 0U))

/*
 * mm_malloc - Allocate a block
 *
 * If there exists a free block where the request fits, get the smallest one, segment it and allocate.
 * If there is no such block, increase brk.
 */
void* mm_malloc(mm_t* mm, size_t size)
{
    size_t block_size, next_block_size;
    void *free_block, *next_block;
    
    block_size = ALIGN(HEADER_SIZE + size);
    block_size = block_size < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : block_size;

    free_block = rb_find(mm, block_size);
    if(free_block == mm->rb_null){ // proper free block not found
        /* set free_block to the end of last block in heap */
        free_block = mem_heap_hi(&(mm->mem)) - 3;
        if(PREV_FREE(free_block)){ // if the last block is free 
            /* set free_block to the last block */
           free_block -= PREV_SIZE_MASKED(free_block);
            if(IS_IN_RB(free_block)){
                rb_delete(mm, free_block);
            }
            /* this block is smaller than request, so increase brk */
            if (mem_sbrk(&(mm->mem), block_size - CUR_SIZE_MASKED(free_block)) == FAIL){
               return NULL;
            }
        }else{ // if the last block is not free
            if (mem_sbrk(&(mm->mem), block_size) == FAIL){
              return NULL;
            }
        }
    }else{
        /* will be allocated, so delete from tree first */
        rb_delete(mm, free_block);
        /* if the block is bigger than request, segment it */
        if((next_block_size = CUR_SIZE_MASKED(free_block) - block_size) > 0){
            next_block = NEXT_BLOCK(free_block, block_size);
            CUR_SIZE(next_block) = PREV_SIZE(NEXT_BLOCK(next_block, next_block_size)) = next_block_size | 1;
            if(IS_IN_RB(next_block)){
                rb_insert(mm, next_block);
            }
        }
    }
    CUR_SIZE(free_block) = PREV_SIZE(NEXT_BLOCK(free_block, block_size)) = block_size;

#if (defined(MM_DEBUG) && (MM_DEBUG > 0U))
    printf("mm_malloc(%u) called\n", size);
    printf("free_block = %p\n", free_block);
    rb_print_preorder(mm);
    printf("\n");
#endif /* MM_DEBUG */

#ifdef MM_CHECK
    if(!mm_check(mm)){
        rb_print_preorder(mm);
        exit(0);
    }
#endif /* MM_CHECK */

    return USER_BLOCK(free_block);
}

/*
 * mm_free - Coalesce with surrounding blocks, and put it to Red-black tree
 */
void mm_free(mm_t* mm, void *ptr)
{
    size_t size, new_size;
    void *prev, *cur, *next, *new_block;

    cur = ptr - HEADER_SIZE;

    /* double free */
    if(CUR_FREE(cur)){
        printf("try to free a freed memory block(%p) is detected.\n", cur);
        return ;
    }

    new_block = cur;
    new_size = CUR_SIZE_MASKED(cur);

    /* coalesce with the previous block if free */
    if(PREV_FREE(cur)){
        size = PREV_SIZE_MASKED(cur);
        prev = PREV_BLOCK(cur, size);
        if(IS_IN_RB(prev)){
            rb_delete(mm, prev);
        }
        new_block = prev;
        new_size += size;
    }

    /* coalesce with the next block if exists and free */
    size = CUR_SIZE_MASKED(cur);
    next = NEXT_BLOCK(cur, size);
    if(next + 4 <= mem_heap_hi(&(mm->mem)) && CUR_FREE(next)){
        size = CUR_SIZE_MASKED(next);
        if(IS_IN_RB(next)){
            rb_delete(mm, next);
        }
        new_size += size;
    }

    /* new free block setting */
    CUR_SIZE(new_block) = PREV_SIZE(NEXT_BLOCK(new_block, new_size)) = new_size | 1;
    if(IS_IN_RB(new_block)){
        rb_insert(mm, new_block);
    }
    
#if (defined(MM_DEBUG) && (MM_DEBUG > 0U))
    printf("mm_free(%p) called\n", ptr);
    printf("new_block = %p\n", new_block);
    rb_print_preorder(mm);
    printf("\n");
#endif /* MM_DEBUG */

#ifdef MM_CHECK
    if(!mm_check(mm)){
        rb_print_preorder(mm);
        exit(0);
    }
#endif /* MM_CHECK */
}

/*
 * mm_exit - finalize the malloc package.
 */
void mm_exit(mm_t* mm)
{
    void *cur, *end;

    cur = mem_heap_lo(&(mm->mem)) + MIN_BLOCK_SIZE;
    end = mem_heap_hi(&(mm->mem)) - 3;
    while(cur < end){
        /* check if there are allocated blocks remaining */
        if(!CUR_FREE(cur)){
            printf("memory leak at %p is detected.\n", cur);
            mm_free(mm, cur + HEADER_SIZE);
        }
        cur = NEXT_BLOCK(cur, CUR_SIZE_MASKED(cur));
    }
}
