
#include <stdio.h>
#include "vm/frame.h"
#include "threads/loader.h"
#include "threads/vaddr.h"
#include "threads/pte.h"
#include "vm/page.h"
#include "userprog/pagedir.h"

struct frame_info *frame_table; /* Global frame table used for storing information
								about the user frames in memory*/
struct lock frame_lock;		/* Synchronization mechanism */
struct list frame_list;		/* Fifo list used for the second chance algorithm*/

void frame_add_map(uint32_t *kpage, struct supp_entry *supp, uint32_t *pagedir,
                 uint32_t *upage)
{
	ASSERT(is_kernel_vaddr(kpage));
    struct frame_info * kframe = &frame_table[FRAME_INDEX(kpage)];
	kframe -> s_entry=supp;
    kframe -> pd = pagedir;
    kframe -> upage = upage;
    list_push_back(&frame_list, &kframe->frame_elem);

    SUPP_SET_CUR_STATE(kframe->s_entry->info_arena, RAM);

}

/* Clear a frame mapping located at the kernel virtual address kpage */
void frame_clear_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));
	/* there is nothing in the frame so have it point to NULL */

	list_remove(&frame_table[FRAME_INDEX(kpage)].frame_elem);
	SUPP_RESET_STICKY(frame_table[FRAME_INDEX(kpage)].s_entry->info_arena);
	frame_table[FRAME_INDEX(kpage)].s_entry=NULL;

}

/* Retrieve frame_info struct located at kernel virtual address kpage*/
struct frame_info *
frame_get_map(uint32_t *kpage)
{
	ASSERT(is_kernel_vaddr(kpage));

	return &frame_table[FRAME_INDEX(kpage)];
}


/**
 * Atomically set a frame table flag
 **/
inline void frame_set_flag(uint32_t *kpage, uint32_t flag)
{
  ASSERT(is_kernel_vaddr(kpage));
  
  /* This is equivalent to `frame_table[FRAME_INDEX(kpage)] |= flag' except that it
     is guaranteed to be atomic on a uniprocessor machine.  See
     the description of the OR instruction in [IA32-v2b]. */
  asm ("orl %1, %0" : "=m" (frame_table[FRAME_INDEX(kpage)]) : "r" (flag) : "cc");
}

/**
 * Atomically unset a frame table flag
 **/
inline void frame_unset_flag(uint32_t *kpage, uint32_t flag)
{
  ASSERT(is_kernel_vaddr(kpage));
  
  /* This is equivalent to `frame_table[FRAME_INDEX(kpage)] &= ~flag' except that it
     is guaranteed to be atomic on a uniprocessor machine.  See
     the description of the AND instruction in [IA32-v2a]. */
  asm ("andl %1, %0" : "=m" (frame_table[FRAME_INDEX(kpage)]) : "r" (~flag) : "cc");
}

/**
 * Causes a page fault on virtual address upage
 **/
inline void page_trigger_fault(uint32_t *upage)
{
  
  asm ("testl %0, %%eax" : /* no outputs */ : "m" (*upage) : "eax", "cc" );
}


void
frame_table_init(struct frame_info *f_table, uint32_t count)
{
	uint32_t i;
	for(i = 0; i<count;i++)
	{
		ASSERT(f_table != NULL)
		f_table->s_entry = NULL;
		f_table++;
	}

}


/* Pin a frame. To be used in conjuction with the system calls to avoid
race conditions */
void frame_pin(uint32_t *pd, uint8_t *start_upage, uint8_t *end_page)
{
	uint32_t *pte;
	uint8_t *round_start = pg_round_down(start_upage);
	uint8_t force_frame = 0;
	char a;

	lock_acquire(&frame_lock);
	while(round_start <= pg_round_down(end_page))
	{

		pte = lookup_page(pd,round_start,false);
		if(*pte!=0)
		{
			if((*pte & PTE_P) != 0)
			{

			//page is in frame
				
				SUPP_SET_STICKY(((struct supp_entry *)(frame_table[FRAME_INDEX(pte_get_page (*pte))].s_entry))->info_arena);


			}
			else
			{
			//page is not in frame so update supp_entry

				SUPP_SET_STICKY(((struct supp_entry *)pagedir_get_ptr(pd,round_start))->info_arena);
			}
		}
		round_start += PGSIZE;
	}
	lock_release(&frame_lock);

	round_start = pg_round_down(start_upage);
	while(round_start <= pg_round_down(end_page))
	{
		//force page to be in memory
		page_trigger_fault(round_start);
		round_start += PGSIZE;
	}

}

/* Unpin the frames that have been previoulsy pinned */
void frame_unpin(uint32_t *pd, uint8_t *start_upage, uint8_t *end_page)
{
	uint32_t *pte;
	uint8_t *round_start = pg_round_down(start_upage);
	lock_acquire(&frame_lock);
	while(round_start <= pg_round_down(end_page))
	{
		pte = lookup_page(pd,round_start,false);
		if(*pte!=0)
		{
			if((*pte & PTE_P) != 0)
			{
			//page is in frame
				SUPP_RESET_STICKY(((struct supp_entry *)(frame_table[FRAME_INDEX(pte_get_page (*pte))].s_entry))->info_arena);
			}
			else
			{
			//page is not in frame so update supp_entry
				SUPP_RESET_STICKY(((struct supp_entry *)pagedir_get_ptr(pd,round_start))->info_arena);
			}
		}
		round_start += PGSIZE;
	}
	lock_release(&frame_lock);
}

