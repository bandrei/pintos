#ifndef USERPROG_PAGEDIR_H
#define USERPROG_PAGEDIR_H

#include <stdbool.h>
#include <stdint.h>

#include "vm/page.h"

#define PAGE_RANGE 2048 /* Maximum stack size allowed to grow*/

uint32_t *pagedir_create (void);
void pagedir_destroy (uint32_t *pd);
bool pagedir_set_page (uint32_t *pd, void *upage, void *kpage, bool rw);
void *pagedir_get_page (uint32_t *pd, const void *upage);
void pagedir_clear_page (uint32_t *pd, void *upage);
bool pagedir_is_dirty (uint32_t *pd, const void *upage);
void pagedir_set_dirty (uint32_t *pd, const void *upage, bool dirty);
bool pagedir_is_accessed (uint32_t *pd, const void *upage);
void pagedir_set_accessed (uint32_t *pd, const void *upage, bool accessed);
void pagedir_set_writable(uint32_t *pd, const void *upage, bool writable);
void pagedir_activate (uint32_t *pd);

void pagedir_set_ptr (uint32_t *pd, const void *vpage, const struct supp_entry *target);
struct supp_entry *pagedir_get_ptr (uint32_t *pd, const void *vpage);

bool pagedir_page_growable(uint32_t *pd, const void *vpage, const uint8_t *esp, bool kcseg);

uint32_t *
lookup_page (uint32_t *pd, const void *vaddr, bool create);

#endif /* userprog/pagedir.h */

