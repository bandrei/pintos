
#include "vm/page.h"
#include <stdio.h>

void init_supp_entry(struct supp_entry *s_entry)
{
	ASSERT(s_entry != NULL);
	s_entry->info_arena = 0;
	s_entry->next = NULL;
}

uint32_t get_page_location(struct supp_entry *s_entry)
{
	return s_entry->info_arena & 0x00000003U;
}

void set_page_location(struct supp_entry *s_entry, enum location_flag flag)
{
	s_entry->info_arena &= flag;
}
