#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include "threads/malloc.h"
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "vm/frame.h"

#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b
/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list waiting_list;
static struct lock waiting_lock;
struct lock file_lock;
//array of priorities containing links of threads
//should only contain ready threads (i.e. THREAD_READY)
static struct list priority_list [PRI_MAX+1];  

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
/* Priority scheduling */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

fixed la_past_weight;
fixed la_cur_weight;
fixed fp_pri_max;

/* BSD */
static volatile fixed load_avg;
static volatile fixed ready_threads;

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

inline static tid_t
_thread_create (const char *name, int priority,
               thread_func *function, void *aux, struct thread **t_ref);

void thread_tick_ps (int64_t ticks);
void thread_tick_mlfqs (int64_t ticks);
void thread_set_priority_mlfqs (int new_priority);
void thread_set_priority_ps (int new_priority);

inline void thread_calc_recent_cpu (struct thread *t, void *aux UNUSED);
inline void thread_calc_priority_mlfqs (struct thread *t, void *aux UNUSED);

//#define READY_THREADS_CHECK 1

#ifdef READY_THREADS_CHECK
inline void thread_count_ready (struct thread *t, void *aux);
#endif

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */

// Macro version for use with an inlineable thread_action_func

#define THREAD_FOREACH(FUNC,AUX) do {			\
  struct list_elem *e;							\
									\
  ASSERT (intr_get_level () == INTR_OFF);				\
									\
  for (e = list_begin (&all_list); e != list_end (&all_list);		\
	e = list_next (e))						\
    {									\
      struct thread *t = list_entry (e, struct thread, allelem);	\
      FUNC (t, AUX);							\
    }									\
									\
} while (0)

void
thread_foreach (thread_action_func *func, void *aux)
{
  THREAD_FOREACH(func,aux);
}

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void) 
{
  /**
   * Initialize appropriate scheduler
   **/
  if (thread_mlfqs) {
    thread_tick = &thread_tick_mlfqs;
    thread_set_priority = &thread_set_priority_mlfqs;
    la_past_weight = FP_DIVI(FP_FROMINT(59),60);
    la_cur_weight = FP_DIVI(FP_FROMINT(1),60);
    fp_pri_max = FP_FROMINT(PRI_MAX);
    load_avg = FP_FROMINT(0);
    
    printf("Past: %d Cur: %d\n",la_past_weight, la_cur_weight);
    
    
  } else {
    thread_tick = &thread_tick_ps;
    thread_set_priority = &thread_set_priority_ps;
  }
  ready_threads = 0;
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  
  list_init (&all_list);
  list_init (&waiting_list);
  list_init (&frame_list);
  lock_init (&waiting_lock);
  lock_init (&frame_lock);

  int i;
  for (i = 0; i <= PRI_MAX; i++)
    list_init(&(priority_list[i]));

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->parent = NULL;
  initial_thread->status = THREAD_RUNNING;
  lock_init(&file_lock);


  ready_threads++;
  if (thread_mlfqs) {
    initial_thread->nice = 0;
    initial_thread->recent_cpu=0;
    thread_calc_priority_mlfqs(initial_thread,NULL);
  }
  initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  idle_thread = NULL;
  _thread_create ("idle", PRI_MIN, idle, &idle_started, &idle_thread);

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick_ps (int64_t ticks) 
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  // wake up everything()
  thread_wake(ticks);
  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}


#ifdef READY_THREADS_CHECK
inline void thread_count_ready (struct thread *t, void *aux) {
  int *count = (int *) aux;
  
  if ((t != idle_thread) && 
    (t->status == THREAD_READY || t->status == THREAD_RUNNING)
  ) {
    (*count)++;
  }
}
#endif

inline void thread_calc_recent_cpu (struct thread *t, void *aux UNUSED) {
  //PRE: has been called on a 1 second interrupt
  fixed x = (load_avg<<1);
  x = FP_DIV(x,FP_ADDI(x,1));
  
  t->recent_cpu = FP_ADDI(FP_MUL(x,t->recent_cpu),t->nice);
}

inline void thread_calc_priority_mlfqs (struct thread *t, void *aux UNUSED) {
  // PRE: Interrupts are off
  
  
#define MLFQS_CALC_PRIORITY FP_CLAMPI(FP_FLOOR( \
    FP_SUB(fp_pri_max, \
      FP_ADD((t->recent_cpu)>>2,FP_FROMINT(t->nice)<<1) \
    ) \
  ),PRI_MIN,PRI_MAX)
  
  int pnew = MLFQS_CALC_PRIORITY;
  
  if (t->status == THREAD_READY) {
    if (pnew != t->priority) {
      // PRE: t->elem is an element of priority_list[t->priority]
      list_remove(&(t->elem));
      t->init_priority = t->priority = pnew;
      list_push_back(&(priority_list[pnew]), &(t->elem));
    }
  } else {
    t->init_priority = t->priority = pnew;
  }
  
  // Floor(PRI_MAX - recent_cpu/4 - nice/2);
}

void
thread_tick_mlfqs (int64_t ticks)
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  
  
  if (t == idle_thread)
    idle_ticks++;
  else {
#ifdef USERPROG
  if (t->pagedir != NULL)
    user_ticks++;
  else
    kernel_ticks++;
#else
    kernel_ticks++;
#endif
    t->recent_cpu = FP_INC(t->recent_cpu);
  }
  
  if (ticks%TIMER_FREQ == 0) {
    
    
    
#ifdef READY_THREADS_CHECK   
	//code used for debugging purposes
	//enable it by defining READY_THREADS_CHECK 
    // Count number of ready threads
    int ready_threads_check = 0;
    THREAD_FOREACH(thread_count_ready,&ready_threads_check);
    // POST: ready_threads = number of active threads
    if (ready_threads_check != ready_threads) {
      printf("!!! ready_threads was %d (should be %d)\n",
	     ready_threads,ready_threads_check);
      ASSERT(false);
    }
#endif    
    
    // Recalculate load average
    load_avg = FP_ADD(FP_MUL(la_past_weight,load_avg),
		      FP_MULI(la_cur_weight,ready_threads));
    
    // Recalculate recent_cpu
    THREAD_FOREACH(thread_calc_recent_cpu,NULL);
  }
  
  if (ticks%4 == 0) {
    THREAD_FOREACH(thread_calc_priority_mlfqs,NULL);
  }

  // wake up everything
  thread_wake(ticks);
  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}



//swap the a thread in the ready list to the
//priority list corresponding to the priority of the
//current thread.
void thread_swap(struct thread *to_swap)
{
	enum intr_level old_level = intr_disable();
	list_remove(&to_swap->elem);
	to_swap->priority = thread_current()->priority;
	list_push_back(&priority_list[thread_current()->priority],
		       &to_swap->elem);
	intr_set_level(old_level);
}

void thread_sleep(int64_t ticks)
{ 
  struct sleeper *sleeper = malloc(sizeof(struct sleeper));
  if(sleeper == NULL){
    // Out of memory, we have to busy wait
      
      while (timer_ticks() < ticks) {
	thread_yield ();
      }
    
  } else {
    sleeper->wake_time = ticks;
    sema_init(&sleeper->waiting_semaphore,0);
    
    
    lock_acquire(&waiting_lock);
    list_insert_ordered(&waiting_list, &sleeper->elem, &sleep_less, NULL);  
    lock_release(&waiting_lock);

    //put the thread to sleep  
    sema_down(&sleeper->waiting_semaphore);

    
    lock_acquire(&waiting_lock);
    list_remove(&sleeper->elem);
    free(sleeper);
    lock_release(&waiting_lock);
  }
}

bool
sleep_less(const struct list_elem *a, const struct list_elem *b,
	   void *aux UNUSED)
{
  return list_entry(a,struct sleeper, elem)->wake_time <  
		list_entry(b,struct sleeper, elem) -> wake_time;
}

inline void thread_wake(int64_t timer_ticks)
{
  //search through the waiting list
  //and pick the elements that are to be woken up
  if(!list_empty(&waiting_list)) {
    struct list_elem *e;
    struct sleeper *tmp_sleeper;
    //iterate throught the list of sleepers and wake up each thread
    //that needs waking (using sema_up). Break the loop early if
    //possible as the list is ordered in ascending order of sleep time
    for (e= list_begin (&waiting_list); e!= list_end (&waiting_list);
	 e = list_next (e)) {	
      tmp_sleeper = list_entry (e, struct sleeper, elem);
      if (tmp_sleeper->wake_time > timer_ticks) {
	break;
      } else {
    		sema_up(&tmp_sleeper->waiting_semaphore); 			
      }
    }
  }
}

/* Prints thread statistics. */
void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) {
  return _thread_create (name, priority,
               function, aux, NULL);
}

/**
 * Guarantees that *t_ref will be set to the new *struct thread
 * before the thread can be scheduled
 **/
inline static tid_t
_thread_create (const char *name, int priority,
               thread_func *function, void *aux, struct thread **t_ref) 
{
  struct thread *t;
  struct thread *parent = NULL;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;
  enum intr_level old_level;

  ASSERT (function != NULL);
  
  parent = thread_current();

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();
  
  /**
   * Not using a function pointer here since it's not that frequently called
   */
  if (thread_mlfqs) {
    /* Inherit BSD scheduling attributes from parent */
    t->nice = parent->nice;
    t->recent_cpu = parent->recent_cpu;
    
    /* Calculate initial priority */
    thread_calc_priority_mlfqs(t, NULL);
  }

  //add child thread to parent thread's list of children
  t->parent = parent;
  t->parent->exec_proc_pid = tid; //used to store info about
  	  	  	  	  	  	  	  	  //the tid returned by the
  	  	  	  	  	  	  	  	  //thread_create() function
  t->child_wait_tid = -1; //this means this process is not yet
  	  	  	  	  	  	  //waiting on any child proc
  struct child_info *info = malloc(sizeof(struct child_info));
  info->child_tid = t->tid;
  info->exit_status = -1;
  info->already_exit = false;
  list_push_back(&parent->children_info, &info->info_elem);
  list_push_back(&parent->children,&t->child_elem);

  t->stack_bottom = PHYS_BASE;


  /* Prepare thread for first run by initializing its stack.
     Do this atomically so intermediate values for the 'stack' 
     member cannot be observed. */
  old_level = intr_disable ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  intr_set_level (old_level);
  
  if (t_ref != NULL) {
    // If we were given a reference to set
    // this occurs before scheduling the thread
    *t_ref = t;
  }
  /* Add to run queue. */
  thread_unblock (t);
  thread_yield();

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);
  
  struct thread * t = thread_current ();
  
  ASSERT(t->status != THREAD_DYING);
  
  if (t->status != THREAD_BLOCKED) {
    t->status = THREAD_BLOCKED;
    
    if (t != idle_thread)
      ready_threads--;
  }
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  list_push_back ( &( priority_list[t->priority] ), &(t->elem));
  //list_push_back (&ready_list, &t->elem);
  t->status = THREAD_READY;
  if (t != idle_thread) ready_threads++;
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void) 
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  ASSERT (!intr_context ());
#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  
  struct thread * t = thread_current();
  
  list_remove (&t->allelem);
  ASSERT(t->status != THREAD_DYING);
  t->status = THREAD_DYING;
  ready_threads--;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    list_push_back ( &( priority_list[cur->priority] ), &cur->elem);

  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

void
thread_set_priority_mlfqs (int new_priority UNUSED)
{
    // Do nothing!
    return;
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority_ps (int new_priority) 
{
  enum intr_level old_level;
  old_level = intr_disable();
  struct thread *cur_thread = thread_current();
  cur_thread->init_priority = new_priority;
  if(list_empty(&cur_thread->lock_list))
  {
    cur_thread->priority = new_priority;
    
  }
  intr_set_level(old_level);
  thread_yield();
}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
  return thread_current ()->priority;
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED) 
{
  (thread_current() -> nice) = FP_CLAMPI(nice,NICE_MIN,NICE_MAX);
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  return (thread_current () -> nice);
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  return FP_ROUND(FP_MULI(load_avg,100));
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  return FP_ROUND(FP_MULI(thread_current () -> recent_cpu, 100));
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  t->is_user_proc = false;
  t->exec_proc_pid = -1;
  strlcpy (t->name, name, sizeof t->name);

  t->stack = (uint8_t *) t + PGSIZE;
  if (!thread_mlfqs) {
	t->init_priority = priority;
    t->priority = priority;
  }
  
  t->try_lock = NULL;
  t->our_file = NULL;
  t->locked_on_file = false;
  list_init(&t->lock_list); //lock_list used for donation
  list_init(&t->children);
  list_init(&t->children_info);
  list_init(&t->files_opened);
  list_init(&t->files_mapped);
  list_init(&t->supp_list);
  sema_init(&t->thread_wait,0);
  sema_init(&t->child_start,0);

  //list_init(&t->children_info_list);
  //sema_init(&t->ready_to_kill,0);
 // sema_init(&t->ready_to_die,0);
  //t->parent_waiting = false;
  t->magic = THREAD_MAGIC;

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
  static struct list * curlist;
  
  struct thread * thread_to_run = NULL;
  int i;
  for (i = PRI_MAX; i >=0; i--) {
    curlist = &(priority_list[i]);
    if(!list_empty(curlist))
      {
	thread_to_run = list_entry (list_pop_front (curlist),
				    struct thread, elem);
	break;
      }
   }
   if(thread_to_run == NULL)
	return idle_thread;
   else
	return thread_to_run;
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
