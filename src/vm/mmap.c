#include "vm/mmap.h"
#include "vm/frame.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/pte.h"
#include "stdio.h"
#include "userprog/pagedir.h"

bool map_file(uint32_t *pd,struct file *fi)
{
	uint8_t *start_address = (uint8_t *)fi->address;
	if(pg_round_down(start_address)!=start_address)
	{
		return false;
	}

	off_t file_size = file_length(fi);
	lock_acquire(&frame_lock);
	//check if all the spots are available
	while(start_address <= pg_round_down(fi->address+file_size))
	{
		if(*(lookup_page(pd,start_address,true)) != 0)
		{
			lock_release(&frame_lock);
			return false;
		}
		start_address += PGSIZE;
	}

	//create supplemental entries and table entries for each mapping
	start_address = (uint8_t*)fi->address;
	struct supp_entry *supp_map;
	struct mmap_entry *mmap_file;
	while(start_address <= pg_round_down(fi->address+file_size))
	{

		//TODO: Do memory checks on malloc
		supp_map = malloc(sizeof(*supp_map));
		init_supp_entry(supp_map);

		mmap_file= malloc(sizeof(*mmap_file));
		mmap_file->page_offset = start_address-(uint8_t *)fi->address;

		supp_map->init_type = FILE;
		supp_map->cur_type = FILE;
		supp_map->table_ptr = mmap_file;

		pagedir_set_ptr(pd,start_address,supp_map);
		start_address += PGSIZE;
	}
	lock_release(&frame_lock);

	return true;
}
void unmap_file(uint32_t *pd, struct file *fi)
{

	lock_acquire(&frame_lock);
	off_t file_size = file_length(fi);
	size_t pages_to_del = (file_size%PGSIZE==0) ? file_size/PGSIZE: file_size/PGSIZE+1;
	size_t pages_to_del_cur = 0;

	struct supp_entry *start_delete;
	//start from the beginning of the mapped address and check if it's in frame
	//or not
	uint8_t *start_address = (uint8_t *)fi->address;
	start_address = (uint8_t*)fi->address;
	struct supp_entry *supp_map;
	struct mmap_entry *mmap_file;
	struct frame_info *f_inf;
	uint32_t *pte;
	while(start_address <= pg_round_down(fi->address+file_size))
	{
		pte = lookup_page(pd,start_address,false);
		//mapping stored in supp entry only
		if( (*pte & PTE_P) == 0U)
		{
			start_delete = pagedir_get_ptr(pd,start_address);
			if(start_delete!= NULL)
			{
				*pte = 0;
				list_remove(&start_delete->supp_elem);
				supp_clear_table_ptr(start_delete);
				free(start_delete);
			}
		}
		else
		{
			//clear the supp_entry the frame is pointing to
			f_inf = frame_get_map(pagedir_get_page(pd,start_address));
			start_delete = f_inf->s_entry;
			list_remove(&start_delete->supp_elem);
			supp_clear_table_ptr(start_delete);
			free(start_delete);
			*pte = 0;
		}

		start_address += PGSIZE;
	}



	lock_release(&frame_lock);
}
