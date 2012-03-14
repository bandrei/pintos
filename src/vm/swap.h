#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "devices/block.h"
#include "lib/kernel/bitmap.h"
#include "threads/vaddr.h"


#define SECTORS_PER_PAGE ( PGSIZE / BLOCK_SECTOR_SIZE )

typedef uint32_t swap_index_t;

/* A swapfile on a block device */
struct swapfile
  {
    struct bitmap * page_map;         /* Bitmap of allocated pages in swap */
    
    struct block * device;            /* Block device swapfile is stored on */
    
    swap_index_t size;                    /* page slot numbers 0 <= x < size */
  };

struct swapfile* swap_create(struct block *target);
void swap_free(struct swapfile *swap);

inline void swap_peek(struct swapfile* swap, swap_index_t slot, void * dest);
void swap_in(struct swapfile* swap, swap_index_t slot, void * dest);
swap_index_t swap_out(struct swapfile* swap, void * src);



#endif