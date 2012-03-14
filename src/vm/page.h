#ifndef SUPP_TABLE_H
#define SUPP_TABLE_H

#include <stdint.h>
#include "threads/thread.h"

/*
 * Enumeration of the flags that can be used
 * to set the information in supp_entry (i.e. by bitwise
 * operations)
 * TODO: extend the flags as necessary (probably more clear
 * to use hex)
 */

enum supp_flag
{
	RAM = 0U,
	SWAP = 1U,
	FILE = 2U,
	EXE = 3U,
	SUPP_ZERO = 4U
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
	union supp_entry_ptr table_ptr;

	//tmp hack until mmap implementation
	uint32_t read_bytes;

	/*use this in conjunction with a list
	 *of supplemental table entries
	 */
	struct supp_entry *next;
};

void init_supp_entry(struct supp_entry *s_entry);
uint32_t supp_get_page_location(struct supp_entry *s_entry);
void supp_set_flag(struct supp_entry *s_entry, enum supp_flag flag);
void supp_set_table_ptr(struct supp_entry *s_entry, void *address);

#endif
