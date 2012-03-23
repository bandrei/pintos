#include "vm/mmap.h"
#include "vm/frame.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/pte.h"
#include "stdio.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"


/* Map the file fi in the page directory pd starting at the address
specified in the file structure */
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

		/* Terminate the thread if malloc is not successful*/
		supp_map = malloc(sizeof(*supp_map));
		if(supp_map == NULL)
		{
			lock_release(&frame_lock);
			_sys_exit(-1,true);
		}
		init_supp_entry(supp_map);

		mmap_file= malloc(sizeof(*mmap_file));
		if(mmap_file == NULL)
		{
			lock_release(&frame_lock);
			_sys_exit(-1,true);
		}
		mmap_file->file_ptr = fi;
		mmap_file->page_offset = start_address-(uint8_t *)fi->address;

		SUPP_SET_CUR_STATE(supp_map->info_arena, FILE);
		SUPP_SET_INIT_STATE(supp_map->info_arena, FILE);
		SUPP_SET_WRITABLE(supp_map->info_arena,WRITABLE);
		supp_map->table_ptr = mmap_file;

		pagedir_set_ptr(pd,start_address,supp_map);
		start_address += PGSIZE;
	}
	lock_release(&frame_lock);

	return true;
}

/* Unmap the file that has been previously mapped (if the file has not been
mapped this function should not be called */
void unmap_file(uint32_t *pd, struct file *fi)
{


	off_t file_size = file_length(fi);

	struct supp_entry *start_delete;
	//start from the beginning of the mapped address and check if it's in frame
	//or not
	uint8_t *start_address = (uint8_t *)fi->address;
	start_address = (uint8_t*)fi->address;
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
			uint32_t *kpage = pagedir_get_page(pd,start_address);

			f_inf = frame_get_map(kpage);
			start_delete = f_inf->s_entry;

			if(pagedir_is_dirty(pd,start_address))
			{
				file_seek(fi,((struct mmap_entry *)start_delete->table_ptr)->page_offset);
				file_write(fi,start_address,PGSIZE);
				pagedir_set_dirty(pd,start_address,false);
			}

			list_remove(&start_delete->supp_elem);
			supp_clear_table_ptr(start_delete);
			free(start_delete);
			palloc_free_page(kpage);
			frame_clear_map(kpage);
			*pte = 0;
		}

		start_address += PGSIZE;
	}



}
