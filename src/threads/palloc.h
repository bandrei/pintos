#ifndef THREADS_PALLOC_H
#define THREADS_PALLOC_H

#include <stddef.h>

/* How to allocate pages. */
enum palloc_flags
  {
    PAL_ASSERT = 1U,           /* Panic on failure. */
    PAL_ZERO = 2U,             /* Zero page contents. */
    PAL_USER = 4U,				/* User page. */
    PAL_SUPP = 8U
  };

void palloc_init (size_t user_page_limit);
void *palloc_get_page (enum palloc_flags);
void *palloc_get_multiple (enum palloc_flags, size_t page_cnt);
void palloc_free_page (void *);
void palloc_free_multiple (void *, size_t page_cnt);

#endif /* threads/palloc.h */
