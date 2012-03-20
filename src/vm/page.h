#ifndef SUPP_TABLE_H
#define SUPP_TABLE_H

#include <stdint.h>
#include "threads/thread.h"
#include "vm/swap.h"

extern struct swapfile *swap_table;

/*
 * Enumeration of the flags that can be used
 * to set the information in supp_entry (i.e. by bitwise
 * operations)
 * TODO: extend the flags as necessary (probably more clear
 * to use hex)
 */

/**
 * info_arena layout
 * 
 * MSB |                ZERO[1bit]|STATE [3bits]| LSB
 **/


#define SUP_STATE_MASK_SET ((~0U)<<3)
#define SUP_STATE_MASK_GET (~SUP_STATE_MASK_SET)

#define SUP_STATE_RAM 0U
#define SUP_STATE_SWAP 1U
#define SUP_STATE_FILE 2U
#define SUP_STATE_EXE 3U

#define SUP_ZERO (1U<<4)

/**
 * Example:
 * SUP_SET_STATE(t->info_arena,SUP_STATE_RAM)
 * if (SUP_GET_STATE(t->info_arena) == SUP_STATE_RAM)
 **/

//#define SUP_SET_STATE(VAL,STATE) VAL = ((VAL & SUP_STATE_MASK_SET) | (STATE & SUP_STATE_MASK_GET))

//#define SUP_GET_STATE(VAL) (VAL & SUP_STATE_MASK_GET)

//#define SUPP_IS_EXE 4U
//#define SUPP_IS_RAM 1U

//#define SUPP_SET_FLAG(VAL, FLAG) (VAL = (VAL | FLAG))
//#define SUPP_UNSET_FLAG(VAL, FLAG) (VAL = (VAL & ~FLAG))
#define SUPP_GET_FLAG(VAL) VAL
#define SUPP_SET_FLAG(VAL, FLAG) (VAL = FLAG)




enum supp_flag
{
	RAM = 0,
	SWAP = 1,
	FILE = 2,
	EXE = 3
	//SUPP_ZERO = 5U
};


union supp_entry_ptr
{
	void *ram_table_entry;
	void *swap_table_entry;
	void *file_table_entry;
	void *exe_table_entry;
};

struct supp_entry
{
	/* store information in this 32 bit number
	 * in a bitwise fashion;
	 */
	//uint32_t info_arena;

	enum supp_flag cur_type;
	enum supp_flag init_type;
	//pointer to where the page is now (i.e. swap, disk, etc.)
	void *table_ptr;
	bool writable;
	//tmp hack until mmap implementation

	/*use this in conjunction with a list
	 *of supplemental table entries
	 */
	struct list_elem supp_elem;
};

void init_supp_entry(struct supp_entry *s_entry);
//uint32_t supp_get_page_location(struct supp_entry *s_entry);
//void supp_set_flag(struct supp_entry *s_entry, enum supp_flag flag);
void supp_set_table_ptr(struct supp_entry *s_entry, void *address);
void supp_clear_table_ptr(struct supp_entry *s_entry);

void swap_init(void);
uintptr_t paging_get_free_frame(void);


void paging_evict(uintptr_t kpage);



#endif
