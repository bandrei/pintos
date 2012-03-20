#include "vm/page.h"
#include <stdio.h>
#include "vm/swap.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"
#include "threads/palloc.h"

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
	//put the s_entry at the head of the list
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

bool paging_get_free_frame()
{
  ASSERT(frame_table != NULL);
  /*uint32_t slot = 0;
  for (; slot < user_max_pages; slot++ )
  {
    if (frame_table[slot].s_entry == NULL)
      return (slot * PGSIZE);
  }*/
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
  
  if (kframe->flags & FRAME_STICKY)
    PANIC ("Attempt to evict a sticky frame");
  
  // Find its supp_entry:
  struct supp_entry * ksup = kframe->s_entry;
  
  //if (SUP_GET_STATE(ksup->info_arena) != SUP_STATE_RAM)
   // PANIC ("Frames supp_entry does not have RAM flag set");
  
  // TODO: better locking here
  
  // Swap out the frame:
  //swap_index_t swapslot = swap_out(swap_table, (void *) ktop);
  
  // Update the supp_entry:
 if (ksup->cur_type==RAM)
 {
	 //if(ksup->init_type == EXE)
		// printf("evicting exe\n");
	 //if(ksup->cur_type == EXE)
		// printf("exe it is \n\n");

		//printf("Evicting: %x  %x ", ksup->cur_type, ksup->init_type);
	 if(ksup->init_type ==RAM)
	 {
	  ksup->cur_type = SWAP;
	  //SUP_SET_STATE(ksup->info_arena, SUP_STATE_SWAP);
	 // printf("\nEVICTING THIS SUPP ENTRY %x\n",ksup);
	 // pagedir_set_ptr(kframe->pd, (void *) kframe->upage, ksup);
	  swap_index_t swapslot = swap_out(swap_table, (void *) kpage);
	  ksup->table_ptr= swapslot;
	 }
	 else if(ksup->init_type == EXE)
	 {
		// printf("\nNOT RAM: EVICTING THIS SUPP ENTRY %x\n",ksup);
		ksup->cur_type = ksup->init_type;
		if(pagedir_is_dirty(kframe->pd,kframe->upage) && ksup->writable)
		{
			ksup->cur_type = SWAP;
			swap_index_t swapslot = swap_out(swap_table, (void *) kpage);
			pagedir_set_dirty(kframe->pd,kframe->upage,false);
		    ksup->table_ptr= swapslot;
		}
		// ksup->table_ptr = kframe;
	 }
	  pagedir_set_ptr(kframe->pd, (void *) kframe->upage, ksup);
	  frame_clear_map((uint32_t *) kpage);
	  palloc_free_page(kpage);

}
else
{

	  printf("frame index %d\n",FRAME_INDEX(kpage));
	  printf("what the hell is this? %x %x\n",ksup->cur_type, ksup->init_type);
}
  
  
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
  uint32_t evict_once = 0;

  //iterate throught the FIFO frame list

  struct list_elem *it;
  struct list_elem *it_end = list_end(&frame_list);
  struct frame_info *fi;

 // printf("is slot %d\n",(frame_table+1) - frame_table );
 it = list_begin(&frame_list);
 while(it!=it_end)
  {


	  fi=list_entry(it,struct frame_info, frame_elem);
	  it = fi->frame_elem.next;
	  if(fi->s_entry!=NULL && ((fi->flags & FRAME_STICKY) == 0))
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


 /* for(it=list_begin(&frame_list); it!=list_end(&frame_list);
		  it=list_next(it))
  {
	  fi=list_entry(it,struct frame_info, frame_elem);
	  if(fi->s_entry!=NULL && (fi->flags & FRAME_STICKY == 0))
	  {
		  if(fi->flags & FRAME_SECOND_CHANCE)
		  {
			  free_slot = fi-frame_table;
			 // list_remove(&fi->frame_elem);
			  paging_evict(FRAME_VADDR((slot)));
			  if(evict_once == 4)
			      			break;
			  evict_once++;

		  }
		  else
		  {
			  fi->flags = (fi->flags | FRAME_SECOND_CHANCE);
			  list_remove(&fi->frame_elem);
			  list_push_back(&frame_list, &fi->frame_elem);

		  }
	  }
  }*/
 /*
  for (; slot < user_max_pages; slot++ )
  {
    
    if ((frame_table[slot].s_entry != NULL) && 
      ((frame_table[slot].flags & FRAME_STICKY) == 0 ))
    {
    	if(frame_table[slot].flags & FRAME_SECOND_CHANCE)
    	{
    		free_slot = slot;
    		paging_evict(FRAME_VADDR((slot)));
    		if(evict_once == 4)
    			break;
    		evict_once++;
    		//break;
    	}
    	else
    	{
   		frame_table[slot].flags = (frame_table[slot].flags | FRAME_SECOND_CHANCE);
    	}
    }
  }*/
  
 // if (free_slot < user_max_pages)
    return evict_once;
 // else
  //  PANIC ("Eviction couldn't evict any frames");
}

void supp_set_table_ptr(struct supp_entry *s_entry, void *address)
{
  /*switch(SUP_GET_STATE(s_entry->info_arena)) {
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
  }*/
}

void supp_clear_table_ptr(struct supp_entry *s_entry)
{

	/*if(SUPP_GET_FLAG(s_entry->info_arena,SUPP_IS_RAM))
	{
#ifdef FRAME_WITH_ADDR
	      struct frame_info *f_info = s_entry->table_ptr;
	      lock_acquire(&frame_lock);
	     // printf(" ADDR 1: %x \n ADDR 2: %x", FRAME_VADDR((f_info-frame_table)), (f_info - frame_table) * 4096 + (uint8_t *)user_start );
	      palloc_free_page(FRAME_VADDR((f_info-frame_table)));
	     // palloc_free_page((f_info - frame_table) * 4096 + (uint8_t *)user_start);
	      lock_release(&frame_lock);
#endif
	      s_entry->table_ptr=NULL;
	}
	else if(!SUPP_GET_FLAG(s_entry->info_arena,SUPP_IS_RAM))
	{
		//clear the swap partition
	}
	else
	{

		 lock_acquire(&frame_lock);
		 free(s_entry->table_ptr);
		 lock_release(&frame_lock);
		 s_entry->table_ptr= NULL;
	}*/
	//all pointers should be not NULL at this point

	/*if(s_entry->cur_type == RAM && s_entry->init_type != RAM)
	{
		printf("TYPE %x\n",s_entry->init_type );
		if(s_entry->table_ptr == NULL)
			printf("WHAT??\n");
		lock_acquire(&frame_lock);
		free(s_entry->table_ptr);
		lock_release(&frame_lock);
	}*/
	/*if(s_entry->init_type != RAM)
	{
		lock_acquire(&frame_lock);
		printf("s_entry ptr %x\n",s_entry->table_ptr);
		if(s_entry->table_ptr != NULL)
		free(s_entry->table_ptr);
		lock_release(&frame_lock);
	}*/
	if(s_entry->cur_type == SWAP)
	{
		//remove from swap
	}
	else if(s_entry->cur_type == RAM)
	{
		//printf don't do anything as pagedir will take care
		//of it (supp_entry might not contain a pointer to supp
		//entry
		//frame_clear_map(s_entry->table_ptr);
		//palloc_free_page(s_entry->table_ptr);
	}
	else if(s_entry->cur_type == EXE)
	{
	//	lock_acquire(&frame_lock);
     	free(s_entry->table_ptr);
	}
	  /*switch(s_entry->init_type)
	  {
	    case RAM:
	    {
	      //no need to free anything at this point
	    	//pagedir destroy will take care of it
	    	//TODO: check if it's better to let pagedir destroy
	    	//do the job, or do it ourselves

	      //TODO:for now, let the pagedir do it and if we decide
	    	//to trade off space then re-enable the code here and
	    	//disable the one in pagedir_destroy
#ifdef FRAME_WITH_ADDR
	      struct frame_info *f_info = s_entry->table_ptr;
	      lock_acquire(&frame_lock);
	     // printf(" ADDR 1: %x \n ADDR 2: %x", FRAME_VADDR((f_info-frame_table)), (f_info - frame_table) * 4096 + (uint8_t *)user_start );
	      palloc_free_page(FRAME_VADDR((f_info-frame_table)));
	     // palloc_free_page((f_info - frame_table) * 4096 + (uint8_t *)user_start);
	      lock_release(&frame_lock);
#endif
	      //frame
	      s_entry->table_ptr=NULL;
	      break;
	    }
	    case SWAP:
	    {

	      lock_acquire(&frame_lock);
	      //TODO: remove from the swap parition as necessary


	      lock_release(&frame_lock);
	      //s_entry->table_ptr.swap_table_entry = NULL;
	      break;
	    }
	    case FILE:
	    {

	      lock_acquire(&frame_lock);
	     // free(s_entry->table_ptr);
	      lock_release(&frame_lock);
	      s_entry->table_ptr = NULL;
	      break;

	    }
	    case EXE:
	    {
	      lock_acquire(&frame_lock);
	      //free(s_entry->table_ptr);
	      lock_release(&frame_lock);
	      s_entry->table_ptr = NULL;
	      break;

	    }
	    default:
	      PANIC("Invalid supp_entry state");
	      break;
	  }*/
}

