
#include <stdio.h>
#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/vaddr.h"
#include "vm/page.h"

struct frame_info *frame_table;
struct lock frame_lock;

void frame_add_map(uint32_t *kpage, struct supp_entry *supp, uint32_t *pagedir)
{
	ASSERT(is_kernel_vaddr(kpage));
    struct frame_info * kframe = &frame_table[FRAME_INDEX(kpage)];
	kframe -> s_entry=supp;
    kframe -> flags=0;
    kframe -> pd = pagedir;

    //re-enable if storing the kpage_addr
#ifdef FRAME_WITH_ADDR
    kframe -> kpage_addr = kpage;
#endif


    //now have the s_entry point to the frame too
    //printf("KFRAME KPAGE: %x \n",kpage);
    //printf("S_ENTRY: %x \n", supp);
    //printf("KFRAME in S_ENTRY: %x \n", kframe->kpage_addr);
    SUP_SET_STATE(kframe -> s_entry->info_arena, SUP_STATE_RAM);
	supp_set_table_ptr(frame_table[FRAME_INDEX(kpage)].s_entry, &frame_table[FRAME_INDEX(kpage)]);

	//frame_table[vtop(kpage)/PGSIZE].s_entry->table_ptr.ram_table_entry = &frame_table[vtop(kpage)/PGSIZE];

}

void frame_clear_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));
	/* there is nothing in the frame so have it point to NULL */
    // TODO: does vtop(kpage)/PGSIZE == kpage/PGSIZE ?
	frame_table[FRAME_INDEX(kpage)].s_entry=NULL;
    frame_table[FRAME_INDEX(kpage)].flags=0;
}

struct frame_info *
frame_get_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));

	return &frame_table[FRAME_INDEX(kpage)];
}

uint32_t frame_get_flags(uintptr_t *kpage)
{
    ASSERT(is_kernel_vaddr(kpage));
    return frame_table[FRAME_INDEX(kpage)].flags;
}

void frame_set_flags(uintptr_t *kpage, uint32_t nflags)
{
    ASSERT(is_kernel_vaddr(kpage));
    frame_table[FRAME_INDEX(kpage)].flags = nflags;
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
