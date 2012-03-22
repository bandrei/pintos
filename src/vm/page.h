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





#define SUPP_GET_CUR_STATE(VAL)  (VAL & 3U)
#define SUPP_GET_INIT_STATE(VAL) ((VAL & 12U) >> 2)

#define SUPP_SET_CUR_STATE(VAL,STATE) (VAL = (VAL & ~3U) | STATE)
#define SUPP_SET_INIT_STATE(VAL,STATE) (VAL  = ((VAL & ~12U) | (STATE << 2)))

#define SUPP_GET_STICKY(VAL) (VAL & 16U)
#define SUPP_SET_STICKY(VAL) (VAL = (VAL | 16U))
#define SUPP_RESET_STICKY(VAL) (VAL = (VAL & ~16U))

#define SUPP_SET_WRITABLE(VAL,SETVAL) (VAL = ((VAL & ~WRITABLE) | SETVAL))
#define SUPP_GET_WRITABLE(VAL) (VAL & 32U)


enum supp_flag
{
	RAM = 0U,
	SWAP = 1U,
	FILE = 2U,
	EXE = 3U,
	WRITABLE = 32U
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
	uint32_t info_arena;

	//pointer to where the page is now (i.e. swap, disk, etc.)
	void *table_ptr;
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
bool paging_get_free_frame(void);


void paging_evict(uintptr_t kpage);



#endif
