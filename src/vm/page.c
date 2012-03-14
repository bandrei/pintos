#include "vm/page.h"
#include <stdio.h>
#include "vm/swap.h"
#include "vm/frame.h"

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

uint32_t supp_get_page_location(struct supp_entry *s_entry)
{
	return s_entry->info_arena & 0x00000003U;
}

void supp_set_flag(struct supp_entry *s_entry, enum supp_flag flag)
{
	s_entry->info_arena |= flag;
}

void paging_init()
{
  block* swap_disk = block_get_role(BLOCK_SWAP);
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
    if (frame_table[slot] == NULL)
      return (slot * PGSIZE);
  }
  // POST: no free frames
  return paging_eviction();
}

void paging_evict(uintptr_t kpage)
{
  ASSERT(is_kernel_vaddr(kpage));
  
  uintptr_t ktop = vtop(kpage);
  
  frame_info * kframe = &frame_table[ktop/PGSIZE];
  
  if (kframe->s_entry == NULL)
    PANIC ("Attempt to evict an empty frame");
  
  if (kframe->flags & FRAME_STICKY)
    PANIC ("Attempt to evict a sticky frame");
  
  if (kframe->s_entry->info_arena | RAM)
    PANIC ("Frames supp_entry has RAM flag set");
  
  swap_index_t swapslot = swap_out(swap_table, (void *) ktop);
  kframe->s_entry->info_arena &= (SWAP | (~0U<<3) );
  
  frame_clear_map(ktop);
  
  
  
  
}

uintptr_t paging_eviction()
{
  ASSERT(frame_table != NULL);
  uint32_t slot = 0;
  uint32_t free_slot = user_max_pages;
  for (; slot < user_max_pages; slot++ )
  {
    
    if (frame_table[slot].flags & FRAME_STICKY == 0 )
    {
      
    }
    if (frame_table[slot] == NULL)
      return (slot * PGSIZE);
  }
  
}

void supp_set_table_ptr(struct supp_entry *s_entry, void *address)
{
	if(s_entry->info_arena & RAM)
	{
		s_entry->table_ptr.ram_table_entry = address;
	}
	else if(s_entry->info_arena & SWAP)
	{
		s_entry->table_ptr.swap_table_entry = address;

	}
	else if(s_entry->info_arena & FILE)
	{
		s_entry->table_ptr.file_table_entry = address;
	}
	else if(s_entry->info_arena & EXE)
	{
		s_entry->table_ptr.exe_table_entry = address;
	}
}
