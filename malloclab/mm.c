/*
 * mm.c
 * Name: Zechen Zhang
 * AndrewID: zechenz
 *
 * Solution:
 * Segregated list is used in this implementation. Totally 12 seg lists are
 * applied. The root pointers of these lists are put at the beginning of
 * the heap so it would be easier to access.
 *
 * List 0-5 are holding a centain block size respectively, from 16 bytes to
 * 56 bytes.
 * List 6-11 are holding a range of sizes in each one: list 6 holds blocks
 * of size 2^6 to 2^7, list 7 holds blocks of size 2^7 to 2^8, etc.. And
 * list 11 holds blocks of size 2^11 to inf.
 * 
 * When being asked for allocation of a certain size, find the list first
 * using function *find_list*. Then search the list for a matching block. If no
 * hit found, turn to larger blocks to seek. For list 0-5, it's turn to list
 * min(list_num + 3, 6) for following search. If a hit is found in the const
 * size lists, the first hit is returned. Otherwise, continue to search for
 * the second hit. If there is a second hit, compare its size with first hit.
 * Return the smaller one.
 *
 * Then write the header for the allocated block. The footers of allocated
 * blocks are removed to improve utilization. The allocation information is
 * stored in next block's header. 
 *
 * When being asked to free a block, rewrite its header and footer, then try
 * to coalesce. If any block is coalesced, remove them from the free list and
 * add the new coalesced larger block into the list it fit in.
 *
 * Blocks are united by a double linked list. The second word and the third 
 * word of a free block store the partial pointer to the next block and
 * previous block respectively. The partial pointers are converted to pointers
 * when they are used.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "contracts.h"

#include "mm.h"
#include "memlib.h"


// Create aliases for driver tests
// DO NOT CHANGE THE FOLLOWING!
#ifdef DRIVER
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif

/*
 *  Logging Functions
 *  -----------------
 *  - dbg_printf acts like printf, but will not be run in a release build.
 *  - checkheap acts like mm_checkheap, but prints the line it failed on and
 *    exits if it fails.
 */

#ifndef NDEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#define checkheap(verbose) do {if (mm_checkheap(verbose)) {  \
                             printf("Checkheap failed on line %d\n", __LINE__);\
                             exit(-1);  \
                        }}while(0)
#else
#define dbg_printf(...)
#define checkheap(...)
#endif

/* Constants and macros */
#define WSIZE 4				/* Word and header/footer size (bytes) */ 
#define DSIZE 8				/* Doubleword size (bytes) */
#define CHUNKSIZE (1<<6)	/* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))	/* Return the larger value of x and y */

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)  
#define FTRP(bp)       ((char *)(bp) + block_size(bp) - DSIZE)

/* Global Variables */
typedef void * ptr;
ptr *seg_list; // Array for the root pointers of the seg_list
static char *heap_listp = 0;
static const unsigned int seg_list_num = 12;
/* 
 * First 6 slots are for constant size blocks, from 16 to 56 bytes.
 * The following 6 slots are for larger blocks, from 2^6-2^7 to 2^11-inf
 */

/* 
 * Functions Prototypes
 */
static inline void* align(const void const* p, unsigned char w); 
static inline int aligned(const void const* p);
static int in_heap(const void* p);
static inline unsigned int get(const void *p); 
static inline void put(const void *p, unsigned int val);
static inline unsigned int pack(size_t size, int alloc);
static inline void add_to_list(ptr bp, size_t size); 
static void checkblock(ptr bp);
static void printblock(ptr bp);


/*
 *  Helper functions
 *  ----------------
 */

// Align p to a multiple of w bytes
static inline void* align(const void const* p, unsigned char w) {
    REQUIRES(p != NULL);
    REQUIRES(in_heap(p));

   return (void*)(((uintptr_t)(p) + (w-1)) & ~(w-1));
}

// Check if the given pointer is 8-byte aligned
static inline int aligned(const void const* p) {
    REQUIRES(p != NULL);
    REQUIRES(in_heap(p));

    return align(p, 8) == p;
}

// Return whether the pointer is in the heap.
static int in_heap(const void* p) {
    return ((p <= mem_heap_hi()) && (p >= mem_heap_lo()));
}

// Read or write a word into a pointer.
static inline unsigned int get(const void *p) {
    REQUIRES(p != NULL);
    REQUIRES(in_heap(p));

	return *(unsigned int *)p;
} 
static inline void put(const void *p, unsigned int val) {
    REQUIRES(p != NULL);
    REQUIRES(in_heap(p));

	*(unsigned int *)p = val;
}

// Return packed size and occupation header
static inline unsigned int pack(size_t size, int alloc) {
	return (size | alloc); // 1 for allocated, 0 for free
}

// Word-pointer converter
static inline ptr word_to_ptr(int w) {
	if (w == 0u) return NULL;
	return (ptr)((unsigned int)w + 0x800000000UL);
}

// Pointer-word converter
static inline unsigned int ptr_to_word(ptr bp) {
	if (bp == NULL) return 0u;
	return (unsigned int)((unsigned long int)bp 
						- 0x800000000UL);
}
/*
 *  Block Functions
 *  ---------------
 *  Process block with .
 */

// Return the size of the given block
static inline unsigned int block_size(ptr block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return (get(HDRP(block)) & 0xFFFFFFF8);
}

// Return true if the block is free, false otherwise
static inline int block_free(ptr block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return !(get(HDRP(block)) & 0x00000001);
}

// Mark the given block's header when it's allocated.
static inline void block_mark(ptr block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    put(HDRP(block), get(HDRP(block)) | 0x00000001);
}

// 
static inline int prev_alloc(ptr bp) {
	return (get(HDRP(bp)) & 0x2);
}
// Return the pointer to the previous block
static inline ptr block_prev(ptr block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return (char *)block - block_size(HDRP(block));
    // Get the block size of a free block from its footer 
}

// Return the pointer to the next block
static inline ptr block_next(ptr block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return (char *)block + block_size(block);
}

/* 
 * set_alloc & set_unalloc:
 * Change the allocation status of current block stored
 * in next block's header
 */
static inline void set_alloc(ptr bp) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

	put(HDRP(block_next(bp)), get(HDRP(block_next(bp))) | 0x2);
}
static inline void set_unalloc(ptr bp) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

	put(HDRP(block_next(bp)), get(HDRP(block_next(bp))) & ~0x2);
}

/*
 * next_linked_block & prev_linked_block:
 * Find the prev or next linked block in a double linked list
 */
static inline ptr next_linked_block(ptr bp) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

	if (get(bp) == 0) return NULL;
	return word_to_ptr(get(bp));
}

static inline ptr prev_linked_block(ptr bp) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

	if (get((char *)bp + WSIZE) == 0) return NULL;
	return word_to_ptr(get((char *)bp + WSIZE));
}

/* 
 *  Key functions
 *  -------------
 *  These functions are the key components for malloc accomplishment.
 */

/*
 * place - placeblock of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */ 
static void place(ptr bp, size_t asize) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

    size_t csize = block_size(bp);
    ptr block_ptr;   

    if ((csize - asize) >= (2 * DSIZE)) { 
		put(HDRP(bp), pack(asize, prev_alloc(bp) | 1));
		block_ptr = block_next(bp);
		put(HDRP(block_ptr), pack(csize-asize, 2));
		put(FTRP(block_ptr), pack(csize-asize, 0));
		set_unalloc(block_ptr);
		add_to_list(block_ptr, csize - asize);
    }
    else { 
		put(HDRP(bp), pack(csize, prev_alloc(bp) | 1));
		set_alloc(bp);
    }
}

// Given a block size, return the number of list it shoule be in
static int find_list(size_t size) {
	int dsize_count = size / DSIZE;
	int i = 6; // start counting from 2^6
	if (dsize_count <= 7) { // i.e. 16 <= asize <= 56
		return dsize_count - 2;
		}
	dsize_count = dsize_count / 8; // Look for larger seg list
	while (((dsize_count / 2) > 0) && (i < 11))
		i++;
	return i;
}

// Remove a free block from the free list
static inline void remove_from_list(ptr bp) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

	ptr prev, next;
	int list_num;
	prev = prev_linked_block(bp);
	next = next_linked_block(bp);
	if (!prev) { // Remove first block in a list
		list_num = find_list(block_size(bp));
		seg_list[list_num] = word_to_ptr(get(bp));
	} else {
		put(prev, get(bp));
	}
	if (next) { // Not removing last block in a list
		put((char *)next + WSIZE, ptr_to_word(prev));
	}
}

// Add a block to the free list
static inline void add_to_list(ptr bp, size_t size) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

	int list_num = find_list(size);
	if (seg_list[list_num] == NULL) { // Empty list
		seg_list[list_num] = bp;
		put(bp, 0u);
		put((char *)bp + WSIZE, 0u);
	}
	else { // Non-empty list. Insert to the beginning.
		put(bp, ptr_to_word(seg_list[list_num]));
		seg_list[list_num] = bp;
		put((char *)bp + WSIZE, 0u);
		put((char *)next_linked_block(bp) + WSIZE, ptr_to_word(bp));
	}
}
/*
 * first_fit - Use first fit for small blocks and first-two fit for
 * larger blocks
 */
static ptr first_fit(int list_num, size_t size) {
	ptr bp, block_ptr = NULL;
	for (int i = list_num; i < 12; i++) { // loop for seg lists
		bp = seg_list[i];
		while (bp != NULL) { // loop for double linked list
			if (block_size(bp) >= size) {
				if (i < 6) return bp; 
				/* Return first hit if in constant size seg list 
				 * For larger blocks, compare first and second hit,
				 * return the smaller one.
				 */
				if (block_ptr == NULL) block_ptr = bp;
				else {
					if (block_size(bp) <= block_size(block_ptr)) return bp;
					else return block_ptr;
				}
			}
			bp = next_linked_block(bp);
		}
	}
	return block_ptr;
}

/* 
 * seek_block - find a block for malloc
 */
static ptr seek_block(size_t asize) {
	int list_num = find_list(asize);
	ptr block_ptr;
	/* 
	 * For constant size blocks
	 */
	if (list_num < 6) {
		if (seg_list[list_num]) {
			block_ptr = seg_list[list_num];
			remove_from_list(block_ptr);
			return block_ptr;
		} 
		list_num += 3;
		if (list_num > 6) list_num = 6;
		// Small size are adjusted when exact match not found to lower the
		// inner fragment
	}
	block_ptr = first_fit(list_num, asize);
	if (block_ptr == NULL)
		return NULL;
	else {
		remove_from_list(block_ptr); // Remove a block from double linked list
		return block_ptr;
	}
}

/*
 * coalesce - coalesce a free block with its prev and next block if they are
 * free
 */
static ptr coalesce(ptr bp) {
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));

	int prev_status, next_status;
	ptr block_ptr = bp, prev = NULL, next = block_next(bp);
	size_t size, size_prev = 0, size_next = 0, size_total;
	size = block_size(bp);
	prev_status = prev_alloc(bp); // if prev is allocated
	if (!prev_status) { // Prev is free
		prev = block_prev(bp);
		size_prev = block_size(prev);
	}
	next_status = !block_free(next); // if next is allocated
	if (!next_status)
		size_next = block_size(next);

	if (prev_status && next_status) { // Case 1: all allocated
		return bp;
	} else
	if (!prev_status && !next_status) { // Case 2: all free
		block_ptr = prev;
		remove_from_list(prev);
		remove_from_list(next);
		size_total = size + size_prev + size_next;

		put(HDRP(block_ptr), pack(size_total, prev_alloc(block_ptr)));
		put(FTRP(block_ptr), size_total);

		return block_ptr;

	} else
	if (prev_status && !next_status) { // Case 3: next free
		block_ptr = next;
		remove_from_list(next);
		size_total = size + size_next;

		put(HDRP(bp), pack(size_total, 2));
		put(FTRP(bp), size_total);
		
		return bp;
	} else
	if (!prev_status && next_status) { // Case 4: prev free
		block_ptr = prev;
		remove_from_list(block_ptr);
		size_total = size + size_prev;

		put(HDRP(block_ptr), pack(size_total, prev_alloc(block_ptr)));
		put(FTRP(block_ptr), size_total);

		return block_ptr;
	}
	return block_ptr;	
}


// Extend the heap when there is not a large enough free block.
// Return the pointer of the extended heap.
static ptr extend_heap(unsigned int dsize_count) {
	size_t size;
	ptr bp;
	size = dsize_count * DSIZE;
	if ((bp = mem_sbrk(size)) == NULL)
		return NULL;
	put(HDRP(bp), pack(size, prev_alloc(bp))); // Extended header
	put(FTRP(bp), pack(size, 0)); // Extended footer
	put(HDRP(block_next(bp)), pack(0, 1)); // New epilogue
	return coalesce(bp);
}

/*
 *  Malloc Implementation
 *  ---------------------
 *  The following functions deal with the user-facing malloc implementation.
 */

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
	if ((seg_list = align(mem_sbrk(		
		seg_list_num * sizeof(ptr) + 4 * WSIZE), DSIZE))
		== (ptr)-1)
		return -1;
	/* seg_list is used for store ptrs at beginning of the heap */
	heap_listp = (char *)seg_list + seg_list_num * sizeof(ptr);
	memset(seg_list, 0, seg_list_num * sizeof(ptr)); // initialize seg_list
	put(heap_listp, 0); 
	put(heap_listp + 1 * WSIZE, pack(DSIZE, 1)); 
	put(heap_listp + 2 * WSIZE, pack(DSIZE, 1));
	put(heap_listp + 3 * WSIZE, pack(0, 3));
	(heap_listp += (2 * WSIZE));
    return 0;
}

/*
 * malloc - return a pointer to a block of memory with given size
 */
void *malloc (size_t size) {
	size_t asize, extendsize;
	ptr block_ptr;

	if (heap_listp == 0) 
		mm_init();
    checkheap(1);  // Let's make sure the heap is ok!
	
	if (size == 0) 
		return NULL; // Ingore spurious request

	/* Modify the given size to the alloc size. */
	if (size <= DSIZE) {
		asize = 2 * DSIZE; // Smallest block should be 2*DSIZE
	}
	else { /* Alignment for 8-byte */
		asize = DSIZE * ((size + WSIZE + (DSIZE - 1)) / DSIZE);
	}
		
	block_ptr = seek_block(asize);
	if (block_ptr != NULL) {
		place(block_ptr, asize);
		return block_ptr;
	}
	else { // No fitting block found for an asize byte block.	
    	extendsize = MAX(asize, CHUNKSIZE);
		if ((block_ptr = extend_heap(extendsize/DSIZE)) == NULL) {
			return NULL;
#ifdef DEBUG
		{
			printf("Heap Full");
		}
#endif
		}
		place(block_ptr, asize);
		return block_ptr;
	}
}

/*
 * free - free an allocated block
 */
void free (ptr bp) {
	size_t size;
	ptr block_ptr;
    if ((bp == NULL) || !in_heap(bp) || !aligned(bp)) {
        return;
    }
	size = block_size(bp);
	put(HDRP(bp), pack(size, prev_alloc(bp)));
	put(FTRP(bp), pack(size, 0));
	set_unalloc(bp);
	block_ptr = coalesce(bp);
	add_to_list(block_ptr, block_size(block_ptr));
}

/*
 * realloc - realloc a block for different size and maintain the
 * data in original block
 */
void *realloc(void *ptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
      free(ptr);
      return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
      return malloc(size);
    }

    newptr = malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
      return 0;
    }

    /* Copy the old data. */
    oldsize = block_size(ptr);
    if (size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    free(ptr);

    return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 */
void *calloc (size_t nmemb, size_t size) {
	size_t bytes = nmemb * size;
	void *newptr;
	
	newptr = malloc(bytes);
	memset(newptr, 0, bytes);

	return newptr;
}

/* 
 * Debugging Functions
 */

static void checkblock(ptr bp) 
{
    if ((size_t)bp % 8)
	printf("Error: %p is not doubleword aligned\n", bp);
}

// Returns 0 if no errors were found, otherwise returns the error
int mm_checkheap(int verbose) {
    char *bp = heap_listp;

    if (verbose)
	printf("Heap (%p):\n", heap_listp);

    if ((block_size(heap_listp) != DSIZE) || block_free(heap_listp))
	printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; block_size(bp) > 0; bp = block_next(bp)) {
	if (verbose) 
	    printblock(bp);
		checkblock(bp);
    }

    if (verbose)
	printblock(bp);
    if (((block_size(bp)) != 0) || block_free(bp))
	printf("Bad epilogue header\n");
	return 0;
}
/*
static void printchain(ptr bp)
{
	while (bp)
	{
		printblock(bp);
		printf("->");
		bp = word_to_ptr(get(next_linked_block(bp)));
	}
}
*/

static void printblock(ptr bp) 
{
    size_t hsize, fsize = 0;
	int halloc, falloc = 0;
    checkheap(0);
    hsize = block_size(bp);
    halloc = !block_free(bp);
    if (!halloc) {
    	fsize = get(FTRP(bp)) & 0xFFFFFFF8;
    	falloc = get(FTRP(bp)) & 0x1;  
    }

    if (hsize == 0) {
	printf("%p: EOL\n", bp);
	return;
    }

    if (!halloc)
    	printf("%p: header: [%u:%c] footer: [%u:%c]\n", bp, 
				(unsigned int)hsize, 'f', (unsigned int)fsize, 'f'); 
    else printf("%p: header: [%u:%c], allocated\n", bp, (unsigned int)hsize, 'a');
}
