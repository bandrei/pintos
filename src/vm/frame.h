#ifndef FRAME_TABLE_H
#define FRAME_TABLE_H


#include <stdint.h>
#include "threads/loader.h"
#include "vm/page.h"
#include "threads/synch.h"

/*
 * declare the frame table as an arrray of pointers.
 * we will have this point to the pages in either
 * the supplemental page table or the page table itself
 * depending on whether we want to keep track of both
 * the kernel mapping or just the user mappings
 * (technically speaking only the user mappings should
 * be recorded, therefore it is preferable to point this
 * to the supplemental page table which will contain most
 * of the information we need).
 * Use a structure frame_info just so that we can easily
 * modify it in the future (e.g. for using aliasing)
 */

#define FRAME_STICKY 1U
#define FRAME_SECOND_CHANCE 2U
#define FRAME_INDEX(VAL) ((vtop(VAL)/PGSIZE)-(init_ram_pages-user_max_pages))
#define FRAME_VADDR(VAL) (ptov((VAL * PGSIZE)+((init_ram_pages-user_max_pages)* PGSIZE)))
//#define FRAME_WITH_ADDR

extern size_t user_max_pages;
extern size_t kernel_max_pages;
extern uint32_t last_frame_freed;

extern struct frame_info *frame_table;
extern struct lock frame_lock;
extern struct list frame_list;


struct frame_info
{

    struct supp_entry *s_entry;
    uint32_t *pd;
    uint32_t flags;
    uint32_t *upage;

    struct list_elem frame_elem;
};








/* add a pointer to the frame table by taking a *kpage KERNEL
 * virtual address and a *supp supplemental table entry and
 * have supp_entry point to it
 */
void frame_add_map(uint32_t *kpage, struct supp_entry *supp, uint32_t *pagedir, uint32_t *upage);
void frame_clear_map(uint32_t *kpage);
void frame_table_init (struct frame_info *f_table, uint32_t count);
inline void frame_set_flag(uint32_t *kpage, uint32_t flag);
inline void frame_unset_flag(uint32_t *kpage, uint32_t flag);
inline void page_trigger_fault(uint32_t *upage);

void frame_pin(uint32_t *pd, uint8_t *start_page, uint8_t *end_page);
void frame_unpin(uint32_t *pd, uint8_t *start_page, uint8_t *end_page);

/*
 * get the information for frame located at kernel virtual address
 * kpage
 */
struct frame_info *
frame_get_map(uint32_t *kpage);


#endif
