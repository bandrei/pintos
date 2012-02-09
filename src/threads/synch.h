#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>

/* A counting semaphore. */
struct semaphore 
  {
    volatile unsigned value;             /* Current value. */
    struct list waiters;        /* List of waiting threads. */
  };
  
/* Struct to keep a list of threads 
that have been put asleep*/
struct sleeper
{
  int64_t wake_time;
  struct semaphore waiting_semaphore;
  struct list_elem elem;
};

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_up (struct semaphore *);
void sema_self_test (void);


/* Lock. */
struct lock 
  {
    struct thread *holder;      /* Thread holding lock (for debugging). */
    struct semaphore semaphore; /* Binary semaphore controlling access. */
	int priority;
	struct list_elem elem;		/* will e */
  };

void lock_init (struct lock *);
void lock_acquire (struct lock *);
bool lock_try_acquire (struct lock *);
void lock_release (struct lock *);
bool lock_held_by_current_thread (const struct lock *);
void lock_donate(struct lock *);
int lock_donate_restore(struct lock *);
//return the thread priority after donation removal

/* Condition variable. */
struct condition 
  {
    struct list waiters;        /* List of waiting semaphore_elems. */
  };

void cond_init (struct condition *);
void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);
void cond_broadcast (struct condition *, struct lock *);

/* Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
