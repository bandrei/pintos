#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "stdio.h"

void syscall_init (void);
//static int get_user (const uint8_t *uaddr);
//static bool put_user (uint8_t *udst, uint8_t byte);
void _sys_exit (int status, bool msg_print );


#endif /* userprog/syscall.h */
