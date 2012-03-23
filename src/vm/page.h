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
};



struct supp_entry
{
	/* store information in this 32 bit number
	 * in a bitwise fashion; 
	 */
	uint32_t info_arena;

	//pointer to where the page is now (i.e. swap, disk, etc.)
	void *table_ptr;

	/*use this in conjunction with a list
	 *of supplemental table entries
	 */
	struct list_elem supp_elem;
};

void init_supp_entry(struct supp_entry *s_entry);
void supp_set_table_ptr(struct supp_entry *s_entry, void *address);
void supp_clear_table_ptr(struct supp_entry *s_entry);


bool paging_get_free_frame(void);


void paging_evict(uintptr_t kpage);



#endif
