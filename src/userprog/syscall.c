#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"


static void syscall_handler (struct intr_frame *);
void exit(int status);


void
syscall_init (void) 
{
	lock_init(&filesys_lock);
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  if(uaddr >= PHYS_BASE)
  	return -1;

  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}


static int
get_arg(const uint8_t *uaddr)
{
	int total_result=0, result, i;
	for(i=0; i<4; i++)
	{
		result = get_user(((void *)uaddr)+i);
		if(result == -1)
		{
	  		exit(-1);
		}
		total_result += result<<(i*8);
	}
	return total_result;
}



/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "r" (byte));
  return error_code != -1;
}


void halt(void)
{
	power_off();
}

void exit(int status)
{
	struct thread *cur_thread = thread_current();
	printf ("%s: exit(%d)\n", cur_thread->name, status);
	thread_exit();
}

int exec(const char *cmd_line)
{	
	process_execute(cmd_line);
	return 0;
}

int wait(int pid)
{
	return process_wait(pid);
}

bool create(const char *file, unsigned initial_size)
{	
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
	return true;
}

bool
remove(const char *file)
{
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
	return true;
}

int
open(const char *file)
{
	if(file == NULL)
	{
		
	}
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
	return 0;

}

int
filesize(int fd)
{
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
	return 0;
}

int
read(int fd, void *buffer, unsigned size)
{
	if (fd == STDIN_FILENO)
	{
		return input_getc();
	}
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
	return 1;
}

int
write(int fd, void *buffer, unsigned size)
{
	if (fd == STDOUT_FILENO)
	{
		putbuf(buffer, size);
		return size;
	}
	lock_acquire(&filesys_lock);
	// int result = file_write(fd, buffer, size);
	lock_release(&filesys_lock);
	return size;
}

void
seek(int fd, unsigned position)
{
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
}

unsigned
tell(int fd)
{
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
	return (unsigned) 1;
}

void
close(int fd)
{
	lock_acquire(&filesys_lock);
	lock_release(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int sys_code = get_arg((int*)f->esp);
  switch(sys_code)
  {
  	case SYS_HALT:
  	{
  		halt();
  		break;
  	}
  	case SYS_EXIT:
  	{
  		exit(get_arg((int *)f->esp+1));
  		break;
  	}
  	case SYS_EXEC:
  	{
  		f->eax = exec(get_arg((int *)f->esp+1));
  		break;
  	}
  	case SYS_WAIT:
  	{
  		f->eax = wait(get_arg((int *)f->esp+1));
  		break;
  	}
  	case SYS_CREATE:
  	{
  		f->eax = create( get_arg((int *)f->esp+1), get_arg((int *)f->esp+2));
  		break;
  	}
  	case SYS_REMOVE:
  	{
  		f->eax = remove(get_arg((int *)f->esp+1));
  		break;
  	}
  	case SYS_OPEN:
  	{
  		f->eax = open(get_arg((int *)f->esp+1));
  		break;
  	}
  	case SYS_FILESIZE:
  	{
  		f->eax = filesize(get_arg((int *)f->esp+1));
  		break;
  	}
  	case SYS_READ:
  	{
  		f->eax = read(get_arg((int *)f->esp+1), get_arg((int *)f->esp+2), get_arg((int *)f->esp+3));
  		break;
  	}	
  	case SYS_WRITE:
  	{
  		f->eax = write(get_arg((int *)f->esp+1), get_arg((int *)f->esp+2), get_arg((int *)f->esp+3));
  		break;
  	}
  	case SYS_SEEK:
  	{
  		seek( get_arg((int *)f->esp+1), get_arg((int *)f->esp+2));  		
  		break;
  	}
  	case SYS_TELL:
  	{
  		f->eax = tell(get_arg((int *)f->esp+1));
  		break;
  	}
  	case SYS_CLOSE:
  	{
  		close(get_arg((int *)f->esp+1));
  		break;
  	}
  }
}
