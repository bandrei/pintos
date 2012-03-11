#ifndef SUPP_TABLE_H
#define SUPP_TABLE_H

#include <stdint.h>
#include "lib/kernel/list.h"

/*
 * Enumeration of the flags that can be used
 * to set the information in supp_entry (i.e. by bitwise
 * operations)
 * TODO: extend the flags as necessary (probably more clear
 * to use hex)
 */
enum supp_flags
{
	RAM = 001,
	SWAP = 002,
	DISK = 004  //for file mappings
};

struct supp_entry
{
	/* store information in this 32 bit number
	 * in a bitwise fashion;
	 */
	uint32_t info_arena;

	//pointer to where the page is now (i.e. swap, disk, etc.)
	void *location;

	//could put the PTE value in here as well
	//but it is not necessary if the use of the page table
	//is correct

	/*use this in conjuction with a list
	 *of supplemental table entries
	 */
	struct list_elem supp_elem;
};

void init_supp_entry(struct supp_entry *s_entry);

#endif
