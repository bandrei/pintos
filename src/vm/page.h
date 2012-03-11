#ifndef SUPP_TABLE_H
#define SUPP_TABLE_H

#include <stdint.h>

/*
 * Enumeration of the flags that can be used
 * to set the information in supp_entry (i.e. by bitwise
 * operations)
 * TODO: extend the flags as necessary (probably more clear
 * to use hex)
 */

enum location_flag
{
	RAM = 001,
	SWAP = 002,
	FILE = 003  //for file mappings
};

union supp_entry_ptr
{
	void *ram_table_entry;
	void *swap_table_entry;
	void *file_table_entry;
};

struct supp_entry
{
	/* store information in this 32 bit number
	 * in a bitwise fashion;
	 */
	uint32_t info_arena;

	//pointer to where the page is now (i.e. swap, disk, etc.)
	union supp_entry_ptr table_ptr;


	/*use this in conjunction with a list
	 *of supplemental table entries
	 */
	struct supp_entry *next;
};

void init_supp_entry(struct supp_entry *s_entry);
uint32_t get_page_location(struct supp_entry *s_entry);
void set_page_location(struct supp_entry *s_entry, enum location_flag flag);

#endif
