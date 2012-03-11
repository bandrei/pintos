
#include <stdio.h>
#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/vaddr.h"

struct frame_info *frame_table;


void frame_add_map(uint32_t *kpage, struct supp_entry *supp)
{
	ASSERT(is_kernel_vaddr(kpage));

	frame_table[vtop(kpage)/PGSIZE].s_entry=supp;
}

void frame_clear_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));
	/* there is nothing in the frame so have it point to NULL */
	frame_table[vtop(kpage)/PGSIZE].s_entry=NULL;
}

struct frame_info *
frame_get_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));

	return &frame_table[vtop(kpage)/PGSIZE];
}
