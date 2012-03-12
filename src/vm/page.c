#include "vm/page.h"
#include <stdio.h>

void init_supp_entry(struct supp_entry *s_entry)
{
	ASSERT(s_entry != NULL);
	s_entry->info_arena = 0;
	s_entry->next = NULL;

	//put the s_entry at the head of the list
	s_entry->next = thread_current()->supp_table;
	thread_current()->supp_table = s_entry;
}

uint32_t supp_get_page_location(struct supp_entry *s_entry)
{
	return s_entry->info_arena & 0x00000003U;
}

void supp_set_flag(struct supp_entry *s_entry, enum supp_flag flag)
{
	s_entry->info_arena &= flag;
}
