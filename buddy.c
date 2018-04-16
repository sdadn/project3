/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
	struct list_head list;
	/*page index*/
	int index;
	/*address of the page*/
	char *address;
	/*holds order size*/
	int order;
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */
        INIT_LIST_HEAD(&g_pages[i].list);

		g_pages[i].index = i;
		g_pages[i].order = -1;
		g_pages[i].address = PAGE_TO_ADDR(i);
	}


	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}


/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)

{
    //Using ceil function to get the integer.
    //Required -lm in make file.
    int requ_order = ceil(log2(size));

	if (size < 0 || size > 1 << MAX_ORDER) {
		return NULL;
	}

    //Checking to be sure it is smallest.
    if (requ_order < MIN_ORDER) {
        requ_order = MIN_ORDER;
    }

	for (int curr_order = requ_order; curr_order < MAX_ORDER + 1; curr_order++)
	{
		/**
		 * Checking if the list isn't empty
		 * Then finding first free list that isn't empty greater than order
		 * it then gets the page index and assigns the order requested
		 * once that is complete it removes it from the free space.
		 */
		if(!list_empty(&free_area[curr_order]))
		{
			page_t *f_page = list_entry(free_area[curr_order].next, page_t, list);
			int f_page_index = f_page->index;
			f_page->order = requ_order;
			list_del_init(&f_page->list);

			/**
			 * Splitting up the block by getting the page index on the right side
			 * Then giving the buddy on the far right the order of the current order minus 1.
			 * Add that buddy back to the free list and if it is still too big then continue to split more.
			*/
			while (requ_order != curr_order)
			{
				int right_index = ADDR_TO_PAGE(BUDDY_ADDR(PAGE_TO_ADDR(f_page_index), (curr_order - 1) ));
				page_t *right_buddy = &g_pages[right_index];
				right_buddy->order = curr_order - 1;
				list_add(&right_buddy->list, &free_area[curr_order - 1]);
				 curr_order --;  //continue splitting
			}
			return PAGE_TO_ADDR(f_page_index);
		}
	}
	return NULL;
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */

	int index = ADDR_TO_PAGE ( (char*)addr );
	int c_order = g_pages[index].order;
	page_t *delete = &g_pages[index];




	while (c_order < MAX_ORDER)
	{
		page_t *buddy = &g_pages[ADDR_TO_PAGE(BUDDY_ADDR(PAGE_TO_ADDR(delete->index), c_order))]; //get buddy page
		char isFree = 0; //no bool, check if buddy was found

		struct list_head *pos; //iterator
		list_for_each(pos, &free_area[c_order]) //iterate over list in free area of current order

		if (buddy == list_entry(pos, page_t, list)) //check if free block is buddy
			isFree = 1; //buddy found

		if (isFree) //freed page's buddy is also free
		{
			list_del_init(&buddy->list); //remove from free area
			if(buddy->address < delete->address) //get leftmost/first buddy for next iteration
				delete = buddy;

			c_order++; //increase order
			continue;
		}

		delete->order = c_order; //change page order
		list_add(&delete->list, &free_area[c_order]); //add to free list at current order 20
		return;

	}


	delete->order = c_order; //change page order
	list_add(&delete->list, &free_area[c_order]); //add to free list at cur
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
