#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"
#include <list.h>
#include "filesys/file.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* This is my code */

/* Project 2 USERPROG */
struct file_element * find_file(int fd);

 // Child process control block 
struct child_process
{
	int pid;              	/* pid for child */
	int parent_pid;       	/* pid foe parent */

	int load;             	/* Checking the load. 0 1 2 */
	int alive;
	bool wait;
	bool exit;
	int status;           	 /* child status, which is used exit status */

	struct lock wait_lock;
	struct semaphore sema;  	/* Sema for wait <-> exit */
 	struct semaphore sema_load; /* Sema for exec <-> load */
 	struct list_elem elem;
};

struct file_element
{
	int fd;
	struct file *file;

	struct list_elem elem;
};
/* USERPROG2 */


#endif /* userprog/process.h */
