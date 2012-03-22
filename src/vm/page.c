#include "vm/page.h"
#include <stdio.h>
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/mmap.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "lib/kernel/bitmap.h"

uintptr_t paging_eviction(void);

struct swapfile *swap_table = NULL;

void init_supp_entry(struct supp_entry *s_entry)
{
	ASSERT(s_entry != NULL);
	s_entry->table_ptr = NULL;
	s_entry->cur_type = RAM;
	s_entry->init_type = RAM;
	s_entry->writable = true;
	s_entry->pin = false;

	list_push_front(&thread_current()->supp_list,&s_entry->supp_elem);

}


void swap_init()
{
  struct block* swap_disk = block_get_role(BLOCK_SWAP);
  if (swap_disk == NULL)
    PANIC ("No swap disk found");
  
  swap_table = swap_create(swap_disk);
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
  
  if (kframe->s_entry->pin)
    PANIC ("Attempt to evict a sticky frame");
  
  // Find its supp_entry:
  struct supp_entry * ksup = kframe->s_entry;
  
  
  
  
  // Update the supp_entry:
 if (ksup->cur_type==RAM)
 {



	 if(ksup->init_type ==RAM)
	 {


	  if(pagedir_is_dirty(kframe->pd,kframe->upage))
	  {
		  ksup->cur_type = SWAP;
		  swap_index_t swapslot = swap_out(swap_table, (void *) kpage);
		  ksup->table_ptr= swapslot;
	  }
	 }
	 else if(ksup->init_type == EXE)
	 {

		if(ksup->writable)
		{
			ksup->cur_type = SWAP;
			lock_acquire(&file_lock);
			swap_index_t swapslot = swap_out(swap_table, (void *) kpage);
			lock_release(&file_lock);
		    ksup->table_ptr= swapslot;
		}
		else

		if(!ksup->writable)
		{
			ksup->cur_type = ksup->init_type;
		}

	 }
	 else if(ksup->init_type == FILE)
	 {
		 ksup->cur_type = ksup->init_type;
		 if(pagedir_is_dirty(kframe->pd,kframe->upage) && ksup->writable)
		 {
			 ksup->cur_type = FILE;
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
	  if(fi->s_entry!=NULL && !fi->s_entry->pin)
	  {
	  	  if(fi->flags & FRAME_SECOND_CHANCE)
	  	  {
	  		  free_slot = fi-frame_table;
  			  paging_evict(FRAME_VADDR((free_slot)));
  			  if(evict_once == 4)
	      			break;
		      evict_once++;

	  	  }
	  	 else
	  	 {
	  		 list_remove(&fi->frame_elem);
	  		 fi->flags = (fi->flags | FRAME_SECOND_CHANCE);
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

	
	if(s_entry->cur_type == SWAP)
	{
		//remove from swap
	 bitmap_reset(swap_table->page_map,s_entry->table_ptr);
	}
	else if(s_entry->cur_type == RAM)
	{
		//printf don't do anything as pagedir will take care
		//of it (supp_entry might not contain a pointer to supp
		//entry
		
	}
	else if(s_entry->init_type == EXE)
	{

     	free(s_entry->table_ptr);
	}
	else if(s_entry->init_type == FILE)
	{
	
		free(s_entry->table_ptr);
	}
	 
}

