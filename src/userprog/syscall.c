#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "lib/kernel/console.h"

static void syscall_handler (struct intr_frame *);
static bool buffer_read_check (char *buff, size_t n);
static bool buffer_write_check();
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
static void _sys_exit(int status, bool msg_print);
static struct lock file_lock;

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault
occurred. */
static int
get_user (const uint8_t *uaddr)
{
	int result;
	asm ("movl $1f, %0; movzbl %1, %0; 1:"
		: "=&a" (result) : "m" (*uaddr));
	return result;
}


/* Writes BYTE to user address UDST.
UDST must be below PHYS_BASE.
Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
	int error_code;
	asm ("movl $1f, %0; movb %b2, %1; 1:"
		: "=&a" (error_code), "=m" (*udst) : "q" (byte));
	return error_code != -1;
}

//check that the whole size of the buffer is in
//the user space and return true if that is the
//case

static bool
buffer_read_check (char *buff, size_t n)
{
	unsigned int i;
	for(i = 0; i<n; i++)
	{

		if(buff >= PHYS_BASE || get_user(buff) == -1)
			return false;
		buff++;
	}
	return true;
}

static void
_sys_exit (int status, bool msg_print)
{
	struct thread *cur = thread_current();
	bool parent_waiting;
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
	//clear the malloced list of child_info
	struct child_info *info;
	for(it = list_begin(&cur->children_info); it != list_end(&cur->children_info);
			it = list_next(it))
	{
		info = list_entry(it, struct child_info, info_elem);
		list_remove(&info->info_elem);
		free(info);
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
	intr_set_level(old_level);
	if(msg_print)
	{
				printf("%s: exit(%d)\n",cur->name,status);
	}


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
  }

}

static void sys_halt(struct intr_frame *f)
{

	thread_exit();
}

static void sys_exit(struct intr_frame *f)
{
	int *tmp_esp = f->esp;
	tmp_esp++;
	_sys_exit(*tmp_esp,true);
	//f->eax = *tmp_esp;
}

static void sys_exec(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	char *buff_addr = *tmp_esp;
	tid_t pid = process_execute(buff_addr);
	f->eax = pid;
	//thread_exit();
}

static void sys_wait(struct intr_frame *f)
{
	int *tmp_esp = f->esp;
	tmp_esp++;
	f->eax=process_wait(*tmp_esp);

}

static void sys_create(struct intr_frame *f)
{

	thread_exit();
}

static void sys_remove(struct intr_frame *f)
{

	thread_exit();
}

static void sys_open(struct intr_frame *f)
{

	thread_exit();
}

static void sys_filesize(struct intr_frame *f)
{

	thread_exit();
}

static void sys_read(struct intr_frame *f)
{

	thread_exit();
}



static void sys_write(struct intr_frame *f)
{

	int *tmp_esp = f->esp;
	tmp_esp++;
	if(*tmp_esp == 1)
	{
		tmp_esp++;
		char *buff_addr = *tmp_esp;
		tmp_esp++;
		unsigned int buff_size = *tmp_esp;
		if(buffer_read_check(buff_addr,buff_size)){
			putbuf(buff_addr,buff_size);
			f->eax = buff_size;
		}
		else
		{
			f->eax = 0;
			//probably invoke exit() here
			_sys_exit(-1,true);
			//thread_exit();
		}
	}
	else
	{
		//case for writing to a file here
	}
	//hex_dump(0,f->esp,100,true);
	//printf("System call : %d", *tmp_esp++);
	//printf("System fd: %d", *tmp_esp++);
	//printf ("system call!\n");
}

static void sys_seek(struct intr_frame *f)
{

	thread_exit();
}

static void sys_tell(struct intr_frame *f)
{

	thread_exit();
}

static void sys_close(struct intr_frame *f)
{

	thread_exit();
}


