#include "vm/page.h"
#include <stdio.h>
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/mmap.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "lib/kernel/bitmap.h"

/* auxiliary function used for evicting*/
uintptr_t paging_eviction(void);


/* Initialize the fields in a supplemental page table entry*/
void init_supp_entry(struct supp_entry *s_entry)
{
	ASSERT(s_entry != NULL);
	s_entry->table_ptr = NULL;
	s_entry->info_arena = 0;
	SUPP_SET_CUR_STATE(s_entry->info_arena, RAM);
	SUPP_SET_INIT_STATE(s_entry->info_arena, RAM);
	SUPP_SET_WRITABLE(s_entry->info_arena,WRITABLE);


	list_push_front(&thread_current()->supp_list,&s_entry->supp_elem);

}


bool paging_get_free_frame()
{
  ASSERT(frame_table != NULL);

  // POST: no free frames

  return paging_eviction() > 0;
}

/**
 * Evict a specific frame
 **/
void paging_evict(uintptr_t kpagev)
{
  const void * kpage = (const void *) kpagev;
  ASSERT(is_kernel_vaddr(kpage));
  
  uintptr_t ktop = vtop(kpage);
  
  // Look up in the frame table:
  struct frame_info * kframe = &frame_table[FRAME_INDEX(kpage)];
  
  if (kframe->s_entry == NULL)
    PANIC ("Attempt to evict an empty frame");
  
  if (SUPP_GET_STICKY(kframe->s_entry->info_arena))
    PANIC ("Attempt to evict a sticky frame");
  
  // Find its supp_entry:
  struct supp_entry * ksup = kframe->s_entry;
  
  
  
  
  // Update the supp_entry:
 if (SUPP_GET_CUR_STATE(ksup->info_arena)==RAM)
 {


	  pagedir_clear_page(kframe->pd,kframe->upage);
	 if(SUPP_GET_INIT_STATE(ksup->info_arena) ==RAM)
	 {
		/* Copy page to the swap for later use*/
		  SUPP_SET_CUR_STATE(ksup->info_arena, SWAP);
		  swap_index_t swapslot = swap_out(swap_table, (void *) kpage);
		  ksup->table_ptr= swapslot;
	 }
	 else if(SUPP_GET_INIT_STATE(ksup->info_arena) == EXE)
	 {
		/* Swap out executable pages that behave as writable files*/
		if(SUPP_GET_WRITABLE(ksup->info_arena))
		{
			SUPP_SET_CUR_STATE(ksup->info_arena, SWAP);
			lock_acquire(&file_lock);
			swap_index_t swapslot = swap_out(swap_table, (void *) kpage);
			lock_release(&file_lock);
		    ksup->table_ptr= swapslot;
		}
		else

		if(!SUPP_GET_WRITABLE(ksup->info_arena))
		{
			SUPP_SET_CUR_STATE(ksup->info_arena, EXE);
		}

	 }
	 else if(SUPP_GET_INIT_STATE(ksup->info_arena) == FILE)
	 {
		 SUPP_SET_CUR_STATE(ksup->info_arena, FILE);
		 /* If the page has been written to then swap it back to the file
			system */
		 if(pagedir_is_dirty(kframe->pd,kframe->upage) && SUPP_GET_WRITABLE(ksup->info_arena))
		 {
			 SUPP_SET_CUR_STATE(ksup->info_arena, FILE);
			 struct mmap_entry *tmp_file = (struct mmap_entry *)ksup->table_ptr;
			 struct file *f = tmp_file->file_ptr;
			 lock_acquire(&file_lock);
			 file_seek(f, tmp_file->page_offset);
			 file_write(f,kframe->upage, PGSIZE);
			 lock_release(&file_lock);
		 }
	 }
	  pagedir_set_ptr(kframe->pd, (void *) kframe->upage, ksup);
	  frame_clear_map((uint32_t *) kpage);
	  palloc_free_page(kpage);
}
else
{
	/* Catch an error when the info_arena of the supplemental entry is 
	corrupted*/
	PANIC("Undefined supplemental entry type");
}
  
  
}

/**
 * Do eviction and return the KV address of a free frame
 **/
uintptr_t paging_eviction()
{
  ASSERT(frame_table != NULL);
  
  
  uint32_t slot = 0;
  uint32_t free_slot = user_max_pages;
  uint32_t evict_once = 0;

  //iterate throught the FIFO frame list

  struct list_elem *it;
  struct list_elem *it_end = list_end(&frame_list);
  struct frame_info *fi;
 it = list_begin(&frame_list);
 while(it!=it_end)
  {


	  fi=list_entry(it,struct frame_info, frame_elem);
	  it = fi->frame_elem.next;
	  if(fi->s_entry!=NULL && !SUPP_GET_STICKY(fi->s_entry->info_arena))
	  {
	  	  if(!pagedir_is_accessed(fi->pd,fi->upage))
	  	  {
	  		  free_slot = fi-frame_table;
  			  paging_evict(FRAME_VADDR((free_slot)));

			 /* attempt to evict 4 frames in one go for efficiency reasons*/
  			  if(evict_once == 4)
	      			break;
		      evict_once++;

	  	  }
	  	 else
	  	 {
	  		 list_remove(&fi->frame_elem);
	  		 pagedir_set_accessed(fi->pd,fi->upage,false);
	  		 list_push_back(&frame_list, &fi->frame_elem);

	  	  }
	  }



  }
  
    return evict_once;
}

void supp_set_table_ptr(struct supp_entry *s_entry, void *address)
{
  
}

void supp_clear_table_ptr(struct supp_entry *s_entry)
{

	
	if(SUPP_GET_CUR_STATE(s_entry->info_arena) == SWAP)
	{
		//remove from swap
	 bitmap_reset(swap_table->page_map,s_entry->table_ptr);
	}
	else if(SUPP_GET_CUR_STATE(s_entry->info_arena) == RAM)
	{
		//should not do anything in this case
		
	}
	else if(SUPP_GET_CUR_STATE(s_entry->info_arena) == EXE)
	{

     	free(s_entry->table_ptr);
	}
	else if(SUPP_GET_CUR_STATE(s_entry->info_arena) == FILE)
	{
	
		free(s_entry->table_ptr);
	}
	 
}

