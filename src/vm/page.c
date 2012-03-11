
#include "vm/page.h"
#include <stdio.h>

void init_supp_entry(struct supp_entry *s_entry)
{
	ASSERT(s_entry != NULL);
	s_entry->info_arena = 0;
}
