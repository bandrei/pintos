#include "vm/page.h"
#include <stdio.h>
#include "vm/swap.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "threads/palloc.h"

uintptr_t paging_eviction(void);

struct swapfile *swap_table;

void init_supp_entry(struct supp_entry *s_entry)
{
	ASSERT(s_entry != NULL);
	s_entry->info_arena = 0;
	s_entry->next = NULL;
	supp_set_table_ptr(s_entry, NULL);
	//put the s_entry at the head of the list
	s_entry->next = thread_current()->supp_table;
	thread_current()->supp_table = s_entry;
}
/*
uint32_t supp_get_page_location(struct supp_entry *s_entry)
{
	return s_entry->info_arena & 0x00000003U;
}
*/
/*
void supp_set_flag(struct supp_entry *s_entry, enum supp_flag flag)
{
	s_entry->info_arena |= flag;
}*/

void swap_init()
{
  struct block* swap_disk = block_get_role(BLOCK_SWAP);
  if (swap_disk == NULL)
    PANIC ("No swap disk found");
  
  swap_table = swap_create(swap_disk);
}

uintptr_t paging_get_free_frame()
{
  ASSERT(frame_table != NULL);
  uint32_t slot = 0;
  for (; slot < user_max_pages; slot++ )
  {
    if (frame_table[slot].s_entry == NULL)
      return (slot * PGSIZE);
  }
  // POST: no free frames
  return paging_eviction();
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
  struct frame_info * kframe = &frame_table[ktop/PGSIZE];
  
  if (kframe->s_entry == NULL)
    PANIC ("Attempt to evict an empty frame");
  
  if (kframe->flags & FRAME_STICKY)
    PANIC ("Attempt to evict a sticky frame");
  
  // Find its supp_entry:
  struct supp_entry * ksup = kframe->s_entry;
  
  if (SUP_GET_STATE(ksup->info_arena) != SUP_STATE_RAM)
    PANIC ("Frames supp_entry does not have RAM flag set");
  
  // TODO: better locking here
  
  // Swap out the frame:
  swap_index_t swapslot = swap_out(swap_table, (void *) ktop);
  
  // Update the supp_entry:
  SUP_SET_STATE(ksup->info_arena, SUP_STATE_SWAP);
  
  pagedir_set_ptr(kframe->pd, (void *) ktop, ksup);
  
  frame_clear_map((uint32_t *) ktop);
  
}

/**
 * Do eviction and return the KV address of a free frame
 **/
uintptr_t paging_eviction()
{
  ASSERT(frame_table != NULL);
  
  /**
   * Currently we evict everything!!
   **/
  
  uint32_t slot = 0;
  uint32_t free_slot = user_max_pages;
  for (; slot < user_max_pages; slot++ )
  {
    
    if ((frame_table[slot].s_entry != NULL) && 
      ((frame_table[slot].flags & FRAME_STICKY) == 0 ))
    {
      free_slot = slot;
      paging_evict(slot*PGSIZE);
    }
  }
  
  if (free_slot < user_max_pages)
    return free_slot*PGSIZE;
  else
    PANIC ("Eviction couldn't evict any frames");
}

void supp_set_table_ptr(struct supp_entry *s_entry, void *address)
{
  switch(SUP_GET_STATE(s_entry->info_arena)) {
    case SUP_STATE_RAM:
      s_entry->table_ptr.ram_table_entry = address;
      break;
    case SUP_STATE_SWAP:
      s_entry->table_ptr.swap_table_entry = address;
      break;
    case SUP_STATE_FILE:
      s_entry->table_ptr.file_table_entry = address;
      break;
    case SUP_STATE_EXE:
      s_entry->table_ptr.exe_table_entry = address;
      break;
    default:
    	//printf("SUPP ENTRY TRYING TO ACCESS %x \n\n BAD state %x\n",s_entry, s_entry->info_arena);
      PANIC("Invalid supp_entry state");
      break;
  }
}

void supp_clear_table_ptr(struct supp_entry *s_entry)
{
	//all pointers should be not NULL at this point
	  switch(SUP_GET_STATE(s_entry->info_arena))
	  {
	    case SUP_STATE_RAM:
	    {
	      //no need to free anything at this point
	    	//pagedir destroy will take care of it
	    	//TODO: check if it's better to let pagedir destroy
	    	//do the job, or do it ourselves

	      //TODO:for now, let the pagedir do it and if we decide
	    	//to trade off space then re-enable the code here and
	    	//disable the one in pagedir_destroy
#ifdef FRAME_WITH_ADDR
	      /*struct frame_info *f_info = s_entry->table_ptr.ram_table_entry;
	      palloc_free_page(f_info->kpage_addr);*/
#endif
	      s_entry->table_ptr.ram_table_entry=NULL;
	      break;
	    }
	    case SUP_STATE_SWAP:
	    {
	      free(s_entry->table_ptr.swap_table_entry);
	      s_entry->table_ptr.swap_table_entry = NULL;
	      break;
	    }
	    case SUP_STATE_FILE:
	    {
	      free(s_entry->table_ptr.file_table_entry);
	      s_entry->table_ptr.file_table_entry = NULL;
	      break;
	    }
	    case SUP_STATE_EXE:
	    {
	     // free(s_entry->table_ptr.exe_table_entry);
	      s_entry->table_ptr.exe_table_entry = NULL;
	      break;
	    }
	    default:
	      PANIC("Invalid supp_entry state");
	      break;
	  }
}

