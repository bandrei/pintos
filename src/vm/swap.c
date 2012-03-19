#include "vm/swap.h"
#include <debug.h>

#include "threads/malloc.h"

#include "lib/kernel/bitmap.h"

inline swap_index_t swap_free_slot(struct swapfile* swap);
inline void swap_put(struct swapfile* swap, swap_index_t slot, void * src);

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

void swap_in(struct swapfile* swap, swap_index_t slot, void * dest)
{
  swap_peek(swap,slot,dest);
  bitmap_reset(swap->page_map,slot);
}

swap_index_t swap_out(struct swapfile* swap, void * src)
{
  swap_index_t slot = swap_free_slot(swap);

  //printf("We are swapping...we we are swapping!\n");
  //printf("KPAGE IN SWAP %x\n",src);
  //hex_dump(0,src,4096,true);
  swap_put(swap,slot,src);
  return slot;
}
