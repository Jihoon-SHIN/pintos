#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void
halt(void)
{

}

void 
exit(int status)
{

}

pid_t 
exec(const char *cmd_line)
{
	return 0;
}

int 
wait(pid_t pid)
{
	return 0;
}

bool
create(const char *file, unsigned initial_size)
{
	return true;
}

bool
remove(const char *file)
{

}

int
open(const char *file)
{

}

int
filesize(int fd)
{

}

int
read(int fd, void *buffer, unsigned size)
{

}

int
write(int fd, void *buffer, unsigned size)
{

}

void
seek(int fd, unsigned position)
{

}

unsigned
tell(int fd)
{

}

void
close(int fd)
{
	
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int sys_code = *(int *)f->esp;
  switch(sys_code)
  {
  	case SYS_HALT:
  	{

  	}
  	case SYS_EXIT:
  	{

  	}
  	case SYS_EXEC:
  	{

  	}
  	case SYS_WAIT:
  	{

  	}
  	case SYS_CREATE:
  	{

  	}
  	case SYS_REMOVE:
  	{

  	}
  	case SYS_FILESIZE:
  	{

  	}
  	case SYS_READ:
  	{

  	}
  	case SYS_WRITE:
  	{

  	}
  	case SYS_SEEK:
  	{

  	}
  	case SYS_TELL:
  	{

  	}
  	case SYS_CLOSE:
  	{

  	}

  }
}
