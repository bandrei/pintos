#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "vm/page.h"
#include "vm/mmap.h"
#include "vm/frame.h"
#include "filesys/file.h"
/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill(struct intr_frame *);
static void page_fault(struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
 programs.

 In a real Unix-like OS, most of these interrupts would be
 passed along to the user process in the form of signals, as
 described in [SV-386] 3-24 and 3-25, but we don't implement
 signals.  Instead, we'll make them simply kill the user
 process.

 Page faults are an exception.  Here they are treated the same
 way as other exceptions, but this will need to change to
 implement virtual memory.

 Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
 Reference" for a description of each of these exceptions. */
void exception_init(void) {
	/* These exceptions can be raised explicitly by a user program,
	 e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
	 we set DPL==3, meaning that user programs are allowed to
	 invoke them via these instructions. */
	intr_register_int(3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
	intr_register_int(4, 3, INTR_ON, kill, "#OF Overflow Exception");
	intr_register_int(5, 3, INTR_ON, kill,
			"#BR BOUND Range Exceeded Exception");

	/* These exceptions have DPL==0, preventing user processes from
	 invoking them via the INT instruction.  They can still be
	 caused indirectly, e.g. #DE can be caused by dividing by
	 0.  */
	intr_register_int(0, 0, INTR_ON, kill, "#DE Divide Error");
	intr_register_int(1, 0, INTR_ON, kill, "#DB Debug Exception");
	intr_register_int(6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
	intr_register_int(7, 0, INTR_ON, kill,
			"#NM Device Not Available Exception");
	intr_register_int(11, 0, INTR_ON, kill, "#NP Segment Not Present");
	intr_register_int(12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
	intr_register_int(13, 0, INTR_ON, kill, "#GP General Protection Exception");
	intr_register_int(16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
	intr_register_int(19, 0, INTR_ON, kill,
			"#XF SIMD Floating-Point Exception");

	/* Most exceptions can be handled with interrupts turned on.
	 We need to disable interrupts for page faults because the
	 fault address is stored in CR2 and needs to be preserved. */
	intr_register_int(14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void exception_print_stats(void) {
	printf("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void kill(struct intr_frame *f) {
	/* This interrupt is one (probably) caused by a user process.
	 For example, the process might have tried to access unmapped
	 virtual memory (a page fault).  For now, we simply kill the
	 user process.  Later, we'll want to handle page faults in
	 the kernel.  Real Unix-like operating systems pass most
	 exceptions back to the process via signals, but we don't
	 implement them. */

	/* The interrupt frame's code segment value tells us where the
	 exception originated. */
	switch (f->cs) {
	case SEL_UCSEG:
		/* User's code segment, so it's a user exception, as we
		 expected.  Kill the user process.  */
		/* printf ("%s: dying due to interrupt %#04x (%s).\n",
		        thread_name (), f->vec_no, intr_name (f->vec_no));
		intr_dump_frame (f);
		thread_exit ();*/
		_sys_exit(-1, true);

	case SEL_KCSEG:
		/* Kernel's code segment, which indicates a kernel bug.
		 Kernel code shouldn't throw exceptions.  (Page faults
		 may cause kernel exceptions--but they shouldn't arrive
		 here.)  Panic the kernel to make the point.  */
		if (thread_current()->is_user_proc)
			_sys_exit(-1, true);
		else {
			intr_dump_frame(f);
			PANIC("Kernel bug - unexpected interrupt in kernel");
		}
	default:
		/* Some other code segment?  Shouldn't happen.  Panic the
		 kernel. */

		_sys_exit(-1, true);
	}
}

/* Page fault handler.  This is a skeleton that must be filled in
 to implement virtual memory.  Some solutions to task 2 may
 also require modifying this code.

 At entry, the address that faulted is in CR2 (Control Register
 2) and information about the fault, formatted as described in
 the PF_* macros in exception.h, is in F's error_code member.  The
 example code here shows how to parse that information.  You
 can find more information about both of these in the
 description of "Interrupt 14--Page Fault Exception (#PF)" in
 [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void page_fault(struct intr_frame *f) {
	bool not_present; /* True: not-present page, false: writing r/o page. */
	bool write; /* True: access was write, false: access was read. */
	bool user; /* True: access by user, false: access by kernel. */
	void *fault_addr; /* Fault address. */

	/* Obtain faulting address, the virtual address that was
	 accessed to cause the fault.  It may point to code or to
	 data.  It is not necessarily the address of the instruction
	 that caused the fault (that's f->eip).
	 See [IA32-v2a] "MOV--Move to/from Control Registers" and
	 [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
	 (#PF)". */
	asm ("movl %%cr2, %0" : "=r" (fault_addr));

	/* Turn interrupts back on (they were only off so that we could
	 be assured of reading CR2 before it changed). */
	intr_enable();

	/* Count page faults. */
	page_fault_cnt++;

	/* Determine cause. */
	not_present = (f->error_code & PF_P) == 0;
	write = (f->error_code & PF_W) != 0;
	user = (f->error_code & PF_U) != 0;

	/* To implement virtual memory, delete the rest of the function
	 body, and replace it with code that brings in the page to
	 which fault_addr refers. */
	/*printf ("Page fault at %p: %s error %s page in %s context.\n",
	 fault_addr,
	 not_present ? "not present" : "rights violation",
	 write ? "writing" : "reading",
	 user ? "user" : "kernel");*/

    /* attempt to write to a r/o page. kill the process*/
	if (!not_present)
	{
		kill(f);
	}
	if (fault_addr < PHYS_BASE)
	{
		/* Swap in the pages either from the file system or from the swap*/
		lock_acquire(&frame_lock);
		struct supp_entry *tmp_entry;
		tmp_entry = pagedir_get_ptr(thread_current()->pagedir, fault_addr);
		void *upage = pg_round_down(fault_addr);
		if (tmp_entry != NULL)
		{
			if (SUPP_GET_CUR_STATE(tmp_entry->info_arena) == EXE)
			{
				struct mmap_entry *exe_map =
						(struct mmap_entry *) tmp_entry->table_ptr;
				off_t pos = exe_map->file_ptr;
				uint32_t *newpage = palloc_get_page(PAL_USER);
				if (newpage == NULL ||
				    upage >= thread_current()->stack_bottom || 
				    !pagedir_set_page(thread_current()->pagedir, upage, newpage,
						true))
				{
					lock_release(&frame_lock);
					kill(f);
				}


				size_t page_zero_bytes = PGSIZE - exe_map->page_offset;

				lock_acquire(&file_lock);
				file_seek(thread_current()->our_file, pos);
				file_read(thread_current()->our_file, upage,
						exe_map->page_offset);
				lock_release(&file_lock);
				
				memset(upage + exe_map->page_offset, 0, page_zero_bytes);
				pagedir_set_writable(thread_current()->pagedir,upage,
				            SUPP_GET_WRITABLE(tmp_entry->info_arena));
				            
				pagedir_set_dirty(thread_current()->pagedir,upage,false);
				pagedir_set_accessed(thread_current()->pagedir,upage,false);
				lock_release(&frame_lock);

			}
			else if (SUPP_GET_CUR_STATE(tmp_entry->info_arena) == SWAP)
			{
				uint32_t *newpage = palloc_get_page(PAL_USER);
				swap_index_t swap_slot = tmp_entry->table_ptr;
				if (newpage == NULL ||
				    !pagedir_set_page(thread_current()->pagedir, upage, newpage,
						true))
				{
					lock_release(&frame_lock);
					kill(f);
				}
				
				lock_acquire(&file_lock);
				swap_in(swap_table, swap_slot, upage);

				pagedir_set_writable(thread_current()->pagedir,
				    upage,SUPP_GET_WRITABLE(tmp_entry->info_arena));
				    
				lock_release(&file_lock);
				pagedir_set_dirty(thread_current()->pagedir,upage,false);
				pagedir_set_accessed(thread_current()->pagedir,upage,false);
				lock_release(&frame_lock);


			}
			else if(SUPP_GET_CUR_STATE(tmp_entry->info_arena) == FILE)
			{

				uint32_t *newpage = palloc_get_page(PAL_USER);
				struct mmap_entry *file = tmp_entry->table_ptr;
				if(newpage==NULL ||
				    upage >= thread_current()->stack_bottom ||
				    !pagedir_set_page(thread_current()->pagedir, upage, newpage,
										true))
				{
				    lock_release(&frame_lock);
					kill(f);
				}

		

				size_t bytes_read=0;
				lock_acquire(&file_lock);
				file_seek(file->file_ptr,file->page_offset);
				bytes_read=file_read(file->file_ptr,upage,PGSIZE);
				lock_release(&file_lock);
				size_t page_zero_bytes = PGSIZE-bytes_read;
				memset(upage+bytes_read,0,page_zero_bytes);
				pagedir_set_dirty(thread_current()->pagedir,upage,false);
				pagedir_set_accessed(thread_current()->pagedir,upage,false);
				lock_release(&frame_lock);

			}
			else
			{
				PANIC("Unidentified type of page");
				lock_release(&frame_lock);
				kill(f);
			}
		}

		/* Implementation of stack growth*/
		else if (
				(f->cs == SEL_UCSEG) ?
						pagedir_page_growable(thread_current()->pagedir,
								fault_addr, f->esp, false) :
						pagedir_page_growable(thread_current()->pagedir,
								fault_addr, f->esp, true))
		{


			uint32_t *newpage = palloc_get_page(PAL_USER);
			if (!pagedir_set_page(thread_current()->pagedir, upage, newpage,
					true))
			{
				lock_release(&frame_lock);
				kill(f);
			}
			
			thread_current()->stack_bottom = upage;
			pagedir_set_dirty(thread_current()->pagedir,upage,false);
			pagedir_set_accessed(thread_current()->pagedir,upage,false);
			lock_release(&frame_lock);

		}
		else
		{
			lock_release(&frame_lock);
			kill(f);
		}
	}

	else
	{
		kill(f);
	}

}

