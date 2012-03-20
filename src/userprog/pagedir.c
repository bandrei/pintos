#include "userprog/pagedir.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "threads/init.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "vm/page.h"
#include "vm/frame.h"

static uint32_t *active_pd (void);
static void invalidate_pagedir (uint32_t *);

/* Creates a new page directory that has mappings for kernel
   virtual addresses, but none for user virtual addresses.
   Returns the new page directory, or a null pointer if memory
   allocation fails. */
uint32_t *
pagedir_create (void) 
{
  uint32_t *pd = palloc_get_page (0);
  if (pd != NULL)
    memcpy (pd, init_page_dir, PGSIZE);
  return pd;
}

/* Destroys page directory PD, freeing all the pages it
   references. */
void
pagedir_destroy (uint32_t *pd) 
{
  uint32_t *pde;
  if (pd == NULL)
    return;

  ASSERT (pd != init_page_dir);
  lock_acquire(&frame_lock);
  for (pde = pd; pde < pd + pd_no (PHYS_BASE); pde++)
    if (*pde & PTE_P) 
      {
        uint32_t *pt = pde_get_pt (*pde);
        uint32_t *pte;
        
        //No need to iterate as we do it more effciently in
        //_sys_exit using the frame table;

        //Trade-off between space for the frame table
        //and speed of iterating through all of the
        //pte's using locks
#ifndef FRAME_WITH_ADDR

        for (pte = pt; pte < pt + PGSIZE / sizeof *pte; pte++)
          if (*pte & PTE_P) 
          {
        	frame_clear_map(pte_get_page (*pte));
            palloc_free_page (pte_get_page (*pte));

          }

#endif
        palloc_free_page (pt);

      }
  palloc_free_page (pd);
  lock_release(&frame_lock);
}

/* Returns the address of the page table entry for virtual
   address VADDR in page directory PD.
   If PD does not have a page table for VADDR, behavior depends
   on CREATE.  If CREATE is true, then a new page table is
   created and a pointer into it is returned.  Otherwise, a null
   pointer is returned. */
uint32_t *
lookup_page (uint32_t *pd, const void *vaddr, bool create)
{
  uint32_t *pt, *pde;

  ASSERT (pd != NULL);

  /* Shouldn't create new kernel virtual mappings. */
  ASSERT (!create || is_user_vaddr (vaddr));

  /* Check for a page table for VADDR.
     If one is missing, create one if requested. */
  pde = pd + pd_no (vaddr);
  if (*pde == 0) 
    {
      if (create)
        {
          pt = palloc_get_page (PAL_ZERO);
          if (pt == NULL) 
            return NULL; 
      
          *pde = pde_create (pt);
        }
      else
        return NULL;
    }

  /* Return the page table entry. */
  pt = pde_get_pt (*pde);
  return &pt[pt_no (vaddr)];
}

/* Adds a mapping in page directory PD from user virtual page
   UPAGE to the physical frame identified by kernel virtual
   address KPAGE.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   If WRITABLE is true, the new page is read/write;
   otherwise it is read-only.
   Returns true if successful, false if memory allocation
   failed. */
bool
pagedir_set_page (uint32_t *pd, void *upage, void *kpage, bool writable)
{
  uint32_t *pte;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (pg_ofs (kpage) == 0);
  ASSERT (is_user_vaddr (upage));
  ASSERT (vtop (kpage) >> PTSHIFT < init_ram_pages);
  ASSERT (pd != init_page_dir);


  pte = lookup_page (pd, upage, true);

  if (pte != NULL) 
    {
      ASSERT ((*pte & PTE_P) == 0);

      //if the pte entry is 0 then malloc a new sup entry
      //otherwsie it means that a supp_entry already exists
      //for this entry;
      struct supp_entry *s_entry;
      if(*pte == 0)
      {
    	 //printf("new s_entry created\n");
    	  s_entry = malloc(sizeof(*s_entry));
    	  init_supp_entry(s_entry);
    	  //s_entry->table_ptr = &frame_table[FRAME_INDEX(kpage)];
    	// printf("s_entry ptr : %x ", (uint32_t)s_entry);
    	 // printf("table_ptr %x \n",&frame_table[FRAME_INDEX(kpage)]);
      }
      else
      {
    	  s_entry = pagedir_get_ptr(pd, upage);
      }


      frame_add_map((uint32_t *)kpage,s_entry, pd, upage);

      *pte = pte_create_user (kpage, writable);


      //re-synch TLB
      invalidate_pagedir(pd);


        	//convert void pointer to char pointer to be able to
        	//use PGSIZE
      	//add to list of frames


      return true;
    }
  else
    return false;
}

/* Looks up the physical address that corresponds to user virtual
   address UADDR in PD.  Returns the kernel virtual address
   corresponding to that physical address, or a null pointer if
   UADDR is unmapped. */
void *
pagedir_get_page (uint32_t *pd, const void *uaddr) 
{
  uint32_t *pte;

  ASSERT (is_user_vaddr (uaddr));
  
  pte = lookup_page (pd, uaddr, false);
  if (pte != NULL && (*pte & PTE_P) != 0)
    return pte_get_page (*pte) + pg_ofs (uaddr);
  else
    return NULL;
}

/* Marks user virtual page UPAGE "not present" in page
   directory PD.  Later accesses to the page will fault.  Other
   bits in the page table entry are preserved.
   UPAGE need not be mapped. */
void
pagedir_clear_page (uint32_t *pd, void *upage) 
{
  uint32_t *pte;

  ASSERT (pg_ofs (upage) == 0);
  ASSERT (is_user_vaddr (upage));

  pte = lookup_page (pd, upage, false);
  if (pte != NULL && (*pte & PTE_P) != 0)
    {



      *pte &= ~PTE_P;


      invalidate_pagedir (pd);



    }

}

/**
 * Storing pointers in the PTE:
 * 
 * From the IA-32 Spec:
 * 
 * Not Present Page-Directory and Page-Table Entries
 * When the present flag is clear for a page-table or page-directory entry, the 
 * operating system or executive may use the rest of the entry for storage of
 * information such as the location of the page in the disk storage system
 * 
 * Hence we have 31 bits to store arbitrary data in.
 * 
 * We can therefore store a kernel 32-bit pointer, a Kernel Virtual Address, by
 * making the observation that since all such addresses are above PHYS_BASE
 * which is 3GB which is > 2GB the most significant bit will always be 1.
 * 
 * ----------------------------------
 * | Pointer bits [30.....0]      |0|
 * ----------------------------------
 * 31                            1 0 - Presence Bit
 * 
**/

/* Set the PTE for virtual page VPAGE in PD to the pointer target. */
void
pagedir_set_ptr (uint32_t *pd, const void *vpage, const struct supp_entry *target) 
{
    uint32_t *pte = lookup_page (pd, vpage, true);  
    ASSERT (pte != NULL);
      
    ASSERT( !((((uint32_t) target) & 0x80000000U) == 0U));
    /**
    * target[31] The MSB must be 1 i.e. target >= 2GB
    **/

    *pte = ((uint32_t) target) << 1U;
    /**
    * POST: pte[0] the present bit is 0
    * and pte[31..1] = target[30..0]
    **/

    invalidate_pagedir (pd);
}

/* Returns the pointer stored in the PTE for virtual page VPAGE in PD */
struct supp_entry *
pagedir_get_ptr (uint32_t *pd, const void *vpage) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  void *target = NULL;
  if (pte != NULL)
  {
      ASSERT((*pte & PTE_P) == 0U);
      /**
       * pte[0] Present bit must be 0
       **/
      if(*pte!=0)
      target = (struct supp_entry *) ((*pte >> 1U) | 0x80000000U);
      /**
       * POST: target[31] is set to 1 and
       * target[30..0] = pte[31..1]
       **/
  }
  return target;
}


/*
 * Checks if a page address has any pages already existing within RANGE*/
bool pagedir_page_growable(uint32_t *pd, const void *vpage, const uint8_t *esp, bool kcseg)
{
	char *page_check = (char *)pg_round_up(vpage);
	char *upper_base = (char *)PHYS_BASE;
	unsigned int page_range = 1;

	if(kcseg)
		return (page_check >= PHYS_BASE - (PAGE_RANGE * 4096)) &&
				(page_check >= thread_current()->stack_save_sys || page_check >= thread_current()->stack_save_sys-32);
	else
	{
	return (page_check >= PHYS_BASE - (PAGE_RANGE * 4096)) &&
			(vpage >= esp || vpage >= esp-32);
	}
	/*for(page_range = 0; page_range<=PAGE_RANGE;page_range++)
	{
		page_check += PGSIZE;
		printf("page_ addr %x \n", page_check);
		if(page_check == PHYS_BASE) break;

		*pte = lookup_page(pd,page_check,false);
		if(pte!=NULL)
		{
			if(*pte!=0)
			{
				return true;
			}

		}

		page_check -= 2*PGSIZE;
		if(page_check == 0) break;

		*pte = lookup_page(pd,page_check,false);
		if(pte!=NULL)
		{
			if(*pte != 0) return true;
		}

	}*/
	//return true;
}


/* Returns true if the PTE for virtual page VPAGE in PD is dirty,
   that is, if the page has been modified since the PTE was
   installed.
   Returns false if PD contains no PTE for VPAGE. */
bool
pagedir_is_dirty (uint32_t *pd, const void *vpage) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  return pte != NULL && (*pte & PTE_D) != 0;
}

/* Set the dirty bit to DIRTY in the PTE for virtual page VPAGE
   in PD. */
void
pagedir_set_dirty (uint32_t *pd, const void *vpage, bool dirty) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  if (pte != NULL) 
    {
      if (dirty)
        *pte |= PTE_D;
      else 
        {
          *pte &= ~(uint32_t) PTE_D;
          invalidate_pagedir (pd);
        }
    }
}

/* Returns true if the PTE for virtual page VPAGE in PD has been
   accessed recently, that is, between the time the PTE was
   installed and the last time it was cleared.  Returns false if
   PD contains no PTE for VPAGE. */
bool
pagedir_is_accessed (uint32_t *pd, const void *vpage) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  return pte != NULL && (*pte & PTE_A) != 0;
}

/* Sets the accessed bit to ACCESSED in the PTE for virtual page
   VPAGE in PD. */
void
pagedir_set_accessed (uint32_t *pd, const void *vpage, bool accessed) 
{
  uint32_t *pte = lookup_page (pd, vpage, false);
  if (pte != NULL) 
    {
      if (accessed)
        *pte |= PTE_A;
      else 
        {
          *pte &= ~(uint32_t) PTE_A; 
          invalidate_pagedir (pd);
        }
    }
}


void pagedir_set_writable(uint32_t *pd, const void *vpage, bool writable)
{
	 uint32_t *pte = lookup_page (pd, vpage, false);
	  if (pte != NULL)
	    {
	      if (writable)
	        *pte =  (*pte | 2U);
	      else
	        {
	          *pte = (*pte & ~2U);
	          invalidate_pagedir (pd);
	        }
	      invalidate_pagedir (pd);
	    }
}

/* Loads page directory PD into the CPU's page directory base
   register. */
void
pagedir_activate (uint32_t *pd) 
{
  if (pd == NULL)
    pd = init_page_dir;

  /* Store the physical address of the page directory into CR3
     aka PDBR (page directory base register).  This activates our
     new page tables immediately.  See [IA32-v2a] "MOV--Move
     to/from Control Registers" and [IA32-v3a] 3.7.5 "Base
     Address of the Page Directory". */
  asm volatile ("movl %0, %%cr3" : : "r" (vtop (pd)) : "memory");
}

/* Returns the currently active page directory. */
static uint32_t *
active_pd (void) 
{
  /* Copy CR3, the page directory base register (PDBR), into
     `pd'.
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 3.7.5 "Base Address of the Page Directory". */
  uintptr_t pd;
  asm volatile ("movl %%cr3, %0" : "=r" (pd));
  return ptov (pd);
}

/* Seom page table changes can cause the CPU's translation
   lookaside buffer (TLB) to become out-of-sync with the page
   table.  When this happens, we have to "invalidate" the TLB by
   re-activating it.

   This function invalidates the TLB if PD is the active page
   directory.  (If PD is not active then its entries are not in
   the TLB, so there is no need to invalidate anything.) */
static void
invalidate_pagedir (uint32_t *pd) 
{
  if (active_pd () == pd) 
    {
      /* Re-activating PD clears the TLB.  See [IA32-v3a] 3.12
         "Translation Lookaside Buffers (TLBs)". */
      pagedir_activate (pd);
    } 
}
