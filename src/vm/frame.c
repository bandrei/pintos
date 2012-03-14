
#include <stdio.h>
#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/vaddr.h"
#include "vm/page.h"

struct frame_info *frame_table;

void frame_add_map(uint32_t *kpage, struct supp_entry *supp)
{
	ASSERT(is_kernel_vaddr(kpage));
	frame_table[vtop(kpage)/PGSIZE].s_entry=supp;
    frame_table[vtop(kpage)/PGSIZE].flags=0;

	//now have the s_entry point to the frame too
	frame_table[vtop(kpage)/PGSIZE].s_entry->info_arena |= RAM;
	supp_set_table_ptr(frame_table[vtop(kpage)/PGSIZE].s_entry, &frame_table[vtop(kpage)/PGSIZE]);

	//frame_table[vtop(kpage)/PGSIZE].s_entry->table_ptr.ram_table_entry = &frame_table[vtop(kpage)/PGSIZE];

}

void frame_clear_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));
	/* there is nothing in the frame so have it point to NULL */
    // TODO: does vtop(kpage)/PGSIZE == kpage/PGSIZE ?
	frame_table[vtop(kpage)/PGSIZE].s_entry=NULL;
    frame_table[vtop(kpage)/PGSIZE].flags=0;
}

struct frame_info *
frame_get_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));

	return &frame_table[vtop(kpage)/PGSIZE];
}

uint32_t frame_get_flags(uintptr_t *kpage)
{
    ASSERT(is_kernel_vaddr(kpage));
    return frame_table[vtop(kpage)/PGSIZE].flags;
}

void frame_set_flags(uintptr_t *kpage, uint32_t nflags)
{
    ASSERT(is_kernel_vaddr(kpage));
    frame_table[vtop(kpage)/PGSIZE].flags = nflags;
}

void
frame_table_init(struct frame_info *f_table, uint32_t count)
{
	uint32_t i;
	for(i = 0; i<count;i++)
	{
		ASSERT(f_table != NULL)
        // TODO: Use memset, is faster
		f_table->s_entry = NULL;
        f_table->flags = 0;
		f_table++;
	}

}
