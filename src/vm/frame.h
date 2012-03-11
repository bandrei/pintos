#ifndef FRAME_TABLE_H
#define FRAME_TABLE_H


#include <stdint.h>

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
 */

struct frame_info
{
	uint32_t *address;
	tid_t  pid;
};
extern
static struct frame_info frame_table[];

/* add a mapping to the frame table by taking a *kpage KERNEL
 * virtual address and a *upage USER virtual address (i.e.
 * will map the frame indicated by kpage to the address indicated
 * by *upage)
 */
void frame_add_map(uint32_t *kpage, uint32_t *upage);
void frame_clear_map(uint32_t *kpage);

/*
 * get the information for frame located at kernel virtual address
 * kpage
 */
struct frame_info *
frame_get_info(uint32_t *kpage);
#endif
