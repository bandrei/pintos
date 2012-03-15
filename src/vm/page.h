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

#define SUP_SET_STATE(VAL,STATE) VAL = ((VAL & SUP_STATE_MASK_SET) | (STATE & SUP_STATE_MASK_GET))

#define SUP_GET_STATE(VAL) (VAL & SUP_STATE_MASK_GET)



/*
enum supp_flag
{
	RAM = 0U,
	SWAP = 1U,
	FILE = 2U,
	EXE = 3U,
	SUPP_ZERO = 4U
};
*/

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
	uint32_t info_arena;

	//pointer to where the page is now (i.e. swap, disk, etc.)
	union supp_entry_ptr table_ptr;

	//tmp hack until mmap implementation
	uint32_t read_bytes;

	/*use this in conjunction with a list
	 *of supplemental table entries
	 */
	struct supp_entry *next;
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
