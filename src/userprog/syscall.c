#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
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
static void _sys_exit();
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
		if(get_user(buff) == -1)
			return false;
		buff++;
	}
	return true;
}

static void
_sys_exit (int status)
{
	struct thread *cur = thread_current();
	cur->exit_status = status;
	sema_up(&cur->ready_to_die);
	sema_down(&cur->ready_to_kill);
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
	_sys_exit(*tmp_esp);
}

static void sys_exec(struct intr_frame *f)
{

	thread_exit();
}

static void sys_wait(struct intr_frame *f)
{
	int *tmp_esp = f->esp;
	tmp_esp++;
	process_exit(*tmp_esp);
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
			thread_exit();
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


