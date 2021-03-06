#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include "synch.h"
#include <stdint.h>
#include "../devices/timer.h"
#include "fixedpoint.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

#define NICE_MIN -20
#define NICE_MAX 20


extern struct lock file_lock;
/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    int init_priority;			/* Initial priority when donation starts (i.e. priority to revert to 
					   when all of the donations have been removed */
    struct lock *try_lock;		/* Hold the lock the current thread is trying to lock on */
    struct list lock_list;		/* Hold a list of locks that another thread is trying to acquire
					   from the current thread (used in the donation process)*/
    struct list_elem allelem;		/* List element for all threads list. */
    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */
    /* User thread variables*/

    /* initialised to false, set to true if created by process_execute
   	which means that the thread is a user thread*/
    bool is_user_proc;

    /* Children information for implementing the wait system call*/
    struct thread *parent; 		/*the parent of the current thread*/
    struct list children_info; 	/*store a list for post-mortem child info
								recovery*/
    struct list children;  		/*keep a list of children in order to update
    							them as we die */
    struct list_elem child_elem; /*used to store us in the parent's child list*/
    tid_t child_wait_tid;		/* the child on which the parent is currently
    							waiting */
    struct semaphore thread_wait; /* block thread when waiting on a child. This
    							 will be the semaphore that needs to be upped
    							 so that the parent thread can resume running */
    tid_t exec_proc_pid;		/* created child tid/pid */
    struct semaphore child_start;/* wait for child thread to be created */


    /* Files queue*/
    struct file *our_file; //our executable file to be closed when we die
    struct list files_opened;//all of our opened files
    struct list files_mapped;
    bool locked_on_file;//used when operating on files

    /* BSD */
    fixed recent_cpu;
    int nice;

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;  /* Page directory. */
#endif
    struct list supp_list; /* List of supplemental table entries. To be used
							when freeing up memory*/
    uint8_t *stack_bottom;	/* Current bottom of the stack. To be used in checking
							whether faulting addresses are allowed above it*/
    uint8_t *stack_save_sys; /* Save stack pointer when in sys call*/


    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

struct child_info
{
	tid_t child_tid;
	int exit_status;
	bool already_exit;
	struct list_elem info_elem;
};



/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

typedef void fp_thread_tick (int64_t ticks);
fp_thread_tick *thread_tick;

void thread_sleep(int64_t);
void thread_wake(int64_t);
void thread_print_stats (void);
void thread_swap(struct thread *);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);

typedef void fp_thread_set_priority (int new_priority);
fp_thread_set_priority *thread_set_priority;

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);
bool sleep_less(const struct list_elem *a, const struct list_elem *b,
		void *aux UNUSED);

#endif /* threads/thread.h */
