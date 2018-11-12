#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

#define BOTTOM_USER_VADDR ((void *) 0x08048000)

void syscall_init (void);
bool find_parent_thread(int parent_pid); /* Find the parent thread */

struct lock filesys_lock;	/* filesys lock. filesys can execute one by one */
struct lock exec_exit_lock;	/* Synchronizaion of exec and exit */

#endif /* userprog/syscall.h */