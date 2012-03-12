#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "lib/kernel/console.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
static void sys_halt(struct intr_frame*);
static void sys_exit(struct intr_frame*);
static void sys_exec(struct intr_frame*);
static void sys_wait(struct intr_frame*);
static void sys_create(struct intr_frame*);
static void sys_remove(struct intr_frame*);
static void sys_open(struct intr_frame*);
static void sys_filesize(struct intr_frame*);
static void sys_read(struct intr_frame*);
static void sys_write(struct intr_frame*);
static void sys_seek(struct intr_frame*);
static void sys_tell(struct intr_frame*);
static void sys_close(struct intr_frame*);


//check that the whole size of the buffer (tmp_esp) is in
//the user space and return true if that is the
//case

static void
POINTER_CHECK(char *tmp_esp, size_t n)
{
	size_t i = 0;
	if(n>0)//case for buffers and other structures
	{
		for(i=0;i<n;i++)
		{
			if(tmp_esp >= PHYS_BASE)
			{
				printf("We are out here \n");
				_sys_exit(-1,true);
			}
		}
	}//case for strings
	else
	{
		do
			{
			 if(tmp_esp >= PHYS_BASE)
			 {
				 printf("we are out \n");
					_sys_exit(-1,true);
			 }
			 tmp_esp++;

			}while(*tmp_esp != '\0');
	}
}

static void acquire_file_lock()
{
	lock_acquire(&file_lock);
	thread_current()->locked_on_file = true;
}

static void release_file_lock()
{
	lock_release(&file_lock);
	thread_current()->locked_on_file= false;
}
/*
 * Auxiliary function for exiting/terminating a thread
 */
void
_sys_exit (int status, bool msg_print)
{
	struct thread *cur = thread_current();
	bool parent_waiting = false;
	struct semaphore *parent_sema;
	enum intr_level old_level = intr_disable();
	//let the children know that we are dying
	struct thread *child;
	struct list_elem *it;
	for(it=list_begin(&cur->children);it!=list_end(&cur->children);
			it=list_next(it))
	{
		child = list_entry(it,struct thread, child_elem);
		child->parent = NULL;
	}
	//clear the malloc-ed list of child_info
	struct child_info *info;
	it = list_begin(&cur->children_info);
	while(it != list_end(&cur->children_info))
	{
		info = list_entry(it,struct child_info, info_elem);
		it = info->info_elem.next;
		list_remove(&info->info_elem);
		info->info_elem.next = info->info_elem.prev = NULL;
		free(info);
	}
	/*
	 * Close all files here while loop and call to file_close
	 */

	struct file *file;
	it = list_begin(&cur->files_opened);
	while(it != list_end(&cur->files_opened))
	{
		file = list_entry(it,struct file, file_elem);
		it = file->file_elem.next;
		list_remove(&file->file_elem);
		file->file_elem.prev =file->file_elem.next = NULL;
		file_close(file);
	}

	//check if parent exists
	if(cur->parent != NULL)
	{
		//at this point the parent thread might have set
		//its child_wait_tid and traversed the list in order
		//to find out that the child thread has not died yet
		//which means that by now or at some point in the future
		//it will call sema_down on itself

		//update the info of the current thread
		//in the child_info list of the parent
		for(it = list_begin(&cur->parent->children_info); it != list_end(&cur->parent->children_info);
					it = list_next(it))
		{
				info = list_entry(it, struct child_info, info_elem);
				if(info->child_tid == cur->tid)
				{
					info->exit_status= status;
					info->already_exit = true;
					break;
				}
		}
		if(cur->parent->child_wait_tid == cur->tid)
		{
			parent_waiting = true;
			parent_sema = &cur->parent->thread_wait;
		}
	}
	list_remove(&cur->child_elem);
	if(cur->our_file!=NULL)
	{
				    //TODO: check if cur->locked_on_file = true
					//needs to be set here or not (probably not)
					file_close(cur->our_file);
	}
	intr_set_level(old_level);

	if(msg_print)
	{
				printf("%s: exit(%d)\n",cur->name,status);
	}
	if(cur->locked_on_file)
	{
			release_file_lock();
	}

	//allow writing to file
	//if the parent is still waiting (i.e. it has already set
	//its child_wait_tid flag then sema_up it
	if(parent_waiting)
	{
		sema_up(parent_sema);
	}


	thread_exit();
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f)
{

  //get system call number and call appropriate function

  //need to do checks here
	POINTER_CHECK(f->esp, sizeof(int));
	switch(*(unsigned int *)f->esp)
  {
  case SYS_HALT:
	  sys_halt(f);
	  break;
  case SYS_EXIT:
	  sys_exit(f);
	  break;
	  /* Terminate this process. */
  case SYS_EXEC:
	  /* Start another process. */
	  sys_exec(f);
	  break;
  case SYS_WAIT:
	  /* Wait for a child process to die. */
	  sys_wait(f);
	  break;
  case SYS_CREATE:
	  /* Create a file. */
	  sys_create(f);
	  break;
  case   SYS_REMOVE:
	  /* Delete a file. */
	  sys_remove(f);
	  	  break;
  case   SYS_OPEN:
	  /* Open a file. */
	  sys_open(f);
	  	  break;
  case   SYS_FILESIZE:
	  /* Obtain a file's size. */
	  sys_filesize(f);
	  	  break;

  case   SYS_READ:
	  /* Read from a file. */
	  sys_read(f);
	  break;
  case   SYS_WRITE:                  /* Write to a file. */
	  sys_write(f);
	  break;

  case   SYS_SEEK:
	  /* Change position in a file. */
	  sys_seek(f);
	  break;
  case   SYS_TELL:
	  /* Report current position in a file. */
	  sys_tell(f);
	  break;
  case   SYS_CLOSE:
	  /* Close a file. */
	  sys_close(f);
	  break;
  default: _sys_exit(-1,true);
  }

}

static void sys_halt(struct intr_frame *f)
{
	shutdown_power_off();
}

static void sys_exit(struct intr_frame *f)
{
	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp, sizeof(int));
	f->eax = *tmp_esp;
	_sys_exit(*tmp_esp,true);
}

static void sys_exec(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
	char *buff_addr = *tmp_esp;
    POINTER_CHECK(buff_addr,0);
	tid_t pid = process_execute(buff_addr);
	f->eax = pid;
}

static void sys_wait(struct intr_frame *f)
{
	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(int));
	f->eax=process_wait(*tmp_esp);

}

//Create file
static void sys_create(struct intr_frame *f)
{
	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
	char *file_addr = *tmp_esp;
	tmp_esp++; //get the file size required for creation
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
	POINTER_CHECK(file_addr,0);
	if(!filesys_create(file_addr,*((unsigned *)tmp_esp)))
		f->eax = false; //file creation failed
	else f->eax = true; //file creation successful
}

static void sys_remove(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
	char *file_addr = *tmp_esp;
	POINTER_CHECK(file_addr,0);
	if(!filesys_remove(file_addr))
		f->eax = false; //file remove failed
	else f->eax = true; //file remove successful
}

static void sys_open(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
	char *file_addr = *tmp_esp;
	tmp_esp++; //get the file size required for creation
	POINTER_CHECK(file_addr,0);




	acquire_file_lock();

	struct file *opened = filesys_open(file_addr);
	if(opened != NULL)
	{
		opened->fd=list_size(&thread_current()->files_opened)+2;
		list_push_back(&thread_current()->files_opened, &opened->file_elem);
		f->eax = opened->fd;
	}
	else
		f->eax = -1;

	release_file_lock();

}

static void sys_filesize(struct intr_frame *f)
{
	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(int));
	struct file *fi;
	struct list_elem *it;
	f->eax = -1;
	//acquire file lock and perform operations
	acquire_file_lock();
	for(it = list_begin(&thread_current()->files_opened);
			it != list_end(&thread_current()->files_opened);
			it = list_next(it))
	{
		fi = list_entry(it,struct file, file_elem);
		if(fi->fd == *tmp_esp)
		{
			f->eax=inode_length(file_get_inode(fi));
			break;
		}
	}
	release_file_lock();
}

static void sys_read(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	f->eax = -1;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(int));
	int fd = *tmp_esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
	char *buff_addr = *tmp_esp;
	POINTER_CHECK(buff_addr,0);
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
    unsigned int buff_size = *tmp_esp;
	struct file *fi = NULL;
	struct list_elem *it;
	//acquire file lock and perform operations
	acquire_file_lock();
	if(fd==0)
	{
			int i =0;
			for(i=0; i<buff_size;i++)
			{
				*buff_addr = input_getc();
				buff_addr++;
			}
			f->eax = buff_size;
	}
	else
	{
		for(it = list_begin(&thread_current()->files_opened);
				it != list_end(&thread_current()->files_opened);
				it = list_next(it))
		{
			fi = list_entry(it,struct file, file_elem);
			if(fi->fd == fd)
			{

				f->eax = file_read(fi,buff_addr,buff_size);
				break;
			}
		}
	}

	release_file_lock();

}



static void sys_write(struct intr_frame *f)
{
	f->eax = -1;
	int *tmp_esp = f->esp;
	tmp_esp++;
    POINTER_CHECK(tmp_esp,sizeof(int));
    int fd = *tmp_esp;

    tmp_esp++;
    POINTER_CHECK(tmp_esp,sizeof(unsigned));
    char *buff_addr = *tmp_esp;
    POINTER_CHECK(buff_addr,0);
    tmp_esp++;
    POINTER_CHECK(tmp_esp,sizeof(unsigned));
    //split the buffer here if above 300 bytes probably
    unsigned int buff_size = *tmp_esp;

	if(fd == 1)
	{

		putbuf(buff_addr,buff_size);
		f->eax = buff_size;

	}
	else
	{
		if(fd==0)
			_sys_exit(-1,true);
		else //check if the fd is one of the files opened by us
		{
			struct file *fi = NULL;
			struct list_elem *it;
			acquire_file_lock();
			for(it = list_begin(&thread_current()->files_opened);
					it != list_end(&thread_current()->files_opened);
					it = list_next(it))
			{
				fi = list_entry(it,struct file, file_elem);
				if(fi->fd == fd)
				{

					f->eax = file_write(fi,buff_addr,buff_size);
					break;
				}
			}

			release_file_lock();
		}
	}
}

static void sys_seek(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(int));
	int fd = *tmp_esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(unsigned));
	unsigned position= *tmp_esp;
	struct file *fi;
	struct list_elem *it;
	//acquire file lock and perform operations
	acquire_file_lock();
	for(it = list_begin(&thread_current()->files_opened);
			it != list_end(&thread_current()->files_opened);
			it = list_next(it))
	{
		fi = list_entry(it,struct file, file_elem);
		if(fi->fd == fd)
		{
			file_seek(fi,position);
				break;
		}
	}

	release_file_lock();
}

static void sys_tell(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(int));
	f->eax = -1;
	struct file *fi;
	struct list_elem *it;
		//acquire file lock and perform operations
	acquire_file_lock();
    for(it = list_begin(&thread_current()->files_opened);
			it != list_end(&thread_current()->files_opened);
			it = list_next(it))
	{
		fi = list_entry(it,struct file, file_elem);
		if(fi->fd == *tmp_esp)
		{
				f->eax = file_tell(fi);
				break;
		}
	}
	release_file_lock();
}

static void sys_close(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	POINTER_CHECK(tmp_esp,sizeof(int));
	struct file *fi;
	struct list_elem *it;
	//acquire file lock and perform operations
	acquire_file_lock();
	for(it = list_begin(&thread_current()->files_opened);
			it != list_end(&thread_current()->files_opened);
			it = list_next(it))
	{
		fi = list_entry(it,struct file, file_elem);
		if(fi->fd == *tmp_esp)
		{
			list_remove(&fi->file_elem);
			file_close(fi);
				break;
		}
	}
	release_file_lock();
}


