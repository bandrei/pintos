#include "vm/swap.h"
#include <debug.h>

#include "threads/malloc.h"

#include "lib/kernel/bitmap.h"

inline swap_index_t swap_free_slot(struct swapfile* swap);
inline void swap_put(struct swapfile* swap, swap_index_t slot, void * src);
struct swapfile *swap_table = NULL;

/* Free the swap data structured at system shutdown */
void swap_free(struct swapfile *swap) {
  if (swap != NULL) {
    free(swap->page_map);
    free(swap);
  }
}

struct swapfile* swap_create(struct block *target)
{
  
  /**
   * Various sanity checks:
   **/
    ASSERT(SECTORS_PER_PAGE * BLOCK_SECTOR_SIZE == PGSIZE);
    // Block device sector size must be an exact factor of page size
    
    ASSERT(target != NULL);
    
    struct swapfile *swap = malloc(sizeof *swap);
    
    if (swap == NULL)
      PANIC ("Failed to allocate memory for swapfile descriptor");
    
    swap->size = block_size(target) / SECTORS_PER_PAGE;
    
    swap->page_map = bitmap_create (swap->size);
    
    swap->device = target;

    if (swap->page_map == NULL)
      PANIC ("Failed to allocate memory for swapfile map");
    
    return swap;
}


/* Auxiliary function used by swap_in. It copies the page located at slot
to dest */
inline void swap_peek(struct swapfile* swap, swap_index_t slot, void * dest)
{
  ASSERT(swap != NULL);
  ASSERT(swap -> page_map != NULL);
  ASSERT(swap -> device != NULL);
  
  if (slot >= swap->size)
    PANIC ("Attempt to read from an invalid swap slot");
  if (!bitmap_test(swap->page_map,slot))
    PANIC ("Attempt to read from an empty swap slot");
    
  
  block_sector_t sector = slot * SECTORS_PER_PAGE;
  block_sector_t sectorN = (slot+1) * SECTORS_PER_PAGE;
  
  
  
  for (;sector < sectorN;
       sector++,
       dest += BLOCK_SECTOR_SIZE
      )
    block_read (swap->device, sector, dest);
}

/* Auxilary function used by swap_out. It copies the page located at src
to the swap slot slot*/
inline void swap_put(struct swapfile* swap, swap_index_t slot, void * src)
{
  ASSERT(swap != NULL);
  ASSERT(swap -> page_map != NULL);
  ASSERT(swap -> device != NULL);
  
  if (slot >= swap->size)
    PANIC ("Attempt to write to an invalid swap slot");
  if (bitmap_test(swap->page_map,slot))
    PANIC ("Attempt to write to an occupied swap slot");
  
  block_sector_t sector = slot * SECTORS_PER_PAGE;
  block_sector_t sectorN = (slot+1) * SECTORS_PER_PAGE;
  
  for (;sector < sectorN;
       sector++,
       src += BLOCK_SECTOR_SIZE
      )
     block_write(swap->device,sector,src);
  
  bitmap_mark(swap->page_map,slot); 
}

inline swap_index_t swap_free_slot(struct swapfile* swap) {
  
  ASSERT(swap != NULL);
  ASSERT(swap -> page_map != NULL);
  swap_index_t slot = 0;
  for (; slot < swap->size; slot++)
  if (!bitmap_test (swap->page_map, slot))
    return slot;
  
  // POST: slot >= swap->size
  
  PANIC("No free swap slots");
  
}

/* Swap in a page from slot into dest. To be used in conjuction with
the eviction algorithm as well as the page fault handler */
void swap_in(struct swapfile* swap, swap_index_t slot, void * dest)
{
  swap_peek(swap,slot,dest);
  bitmap_reset(swap->page_map,slot);
}

/* Initialize the swap partition. Called at system boot-up*/
void swap_init()
{
  struct block* swap_disk = block_get_role(BLOCK_SWAP);
  if (swap_disk == NULL)
    PANIC ("No swap disk found");
  
  swap_table = swap_create(swap_disk);
}


/* Swap out a page to a free slot in the swap partition */
swap_index_t swap_out(struct swapfile* swap, void * src)
{
  swap_index_t slot = swap_free_slot(swap);

  swap_put(swap,slot,src);
  return slot;
}
