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

struct frame_info
{
	struct supp_entry *s_entry;
	//uint32_t *pte;
	//tid_t pid;
};

extern size_t user_max_pages;

/*
 * TODO: make sure eviction will set the PTE entry to NULL
 * whenever removing a page from the frame
 */

/*
 * The frame array will only have enough entries
 * to store the pointers to user pages
 */
extern struct frame_info *frame_table;



/* add a pointer to the frame table by taking a *kpage KERNEL
 * virtual address and a *supp supplemental table entry and
 * have supp_entry point to it
 */
void frame_add_map(uint32_t *kpage, struct supp_entry *supp);
void frame_clear_map(uint32_t *kpage);
void frame_table_init (struct frame_info *f_table, uint32_t count);

/*
 * get the information for frame located at kernel virtual address
 * kpage
 */
struct frame_info *
frame_get_map(uint32_t *kpage);


#endif
