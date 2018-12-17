#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

#ifdef VM
#include "vm/page.h"
#endif

#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

static void syscall_handler (struct intr_frame *);
void exit(int status);


#ifdef VM
static int mmap_id = 1;
#endif

/* Init function */
void
syscall_init (void) 
{
	lock_init(&filesys_lock);
	lock_init(&exec_exit_lock);
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

/* Do get user by 4 byte unit */
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

/* I don't use this */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "r" (byte));
  return error_code != -1;
}

/* Syscall function */
void halt(void)
{
	power_off();
}

void exit(int status)
{
	if(!lock_held_by_current_thread(&exec_exit_lock))
		lock_acquire(&exec_exit_lock);

	struct thread *cur_thread = thread_current();
	if(find_parent_thread(cur_thread->child_p->parent_pid))
  	{
  	 	cur_thread->child_p->status = status;
  	}
	printf ("%s: exit(%d)\n", cur_thread->name, status);
	lock_release(&exec_exit_lock);
	thread_exit();
}

int exec(const char *cmd_line)
{	
	lock_acquire(&exec_exit_lock);
	int pid_p = process_execute(cmd_line);
	struct child_process *child_p = find_child_process(pid_p);

	sema_down(&child_p->sema_load);
	if(!child_p->load)
	{
		lock_release(&exec_exit_lock);
		return -1;
	}
	lock_release(&exec_exit_lock);
	return pid_p;
}

int wait(int pid)
{
	return process_wait(pid);
}

bool create(const char *file, unsigned initial_size)
{	
	if(file==NULL)
	{
		exit(-1);
	}
	lock_acquire(&filesys_lock);
	bool result = filesys_create(file, initial_size, false);
	lock_release(&filesys_lock);
	return result;
}

bool
remove(const char *file)
{
	lock_acquire(&filesys_lock);
	bool result = filesys_remove(file);
	lock_release(&filesys_lock);
	return result;
}

int
open(const char *file)
{
	if(file == NULL)
		return -1;
	
	lock_acquire(&filesys_lock);
	struct file* f = filesys_open(file);
	lock_release(&filesys_lock);

	if(f== NULL)
		return -1;

	struct file_element *fe = malloc(sizeof(struct file_element));
	fe->file = f;
	fe->fd = thread_current()->fd;
	list_push_back(&thread_current()->file_list, &fe->elem);
	return thread_current()->fd++;;
}

int
filesize(int fd)
{
	struct file_element *fe = find_file(fd);
	if(fe == NULL)
		return -1;

	lock_acquire(&filesys_lock);
	int size = file_length(fe->file);
	lock_release(&filesys_lock);
	return size;
}

int
read(int fd, void *buffer, unsigned size)
{
	if (fd == STDIN_FILENO)
	{
		return input_getc();
	}
	struct file_element *fe = find_file(fd);
	if(fe==NULL)
		return -1;

	lock_acquire(&filesys_lock);
	int result = file_read(fe->file, buffer, size);
	lock_release(&filesys_lock);
	return result;
}

int
write(int fd, void *buffer, unsigned size)
{
	if (fd == STDOUT_FILENO)
	{
		putbuf(buffer, size);
		return size;
	}
	struct file_element *fe = find_file(fd);
	if(fe==NULL)
		return -1;
	lock_acquire(&filesys_lock);
	int result = file_write(fe->file, buffer, size);
	lock_release(&filesys_lock);
	return result;
}

void
seek(int fd, unsigned position)
{
	struct file_element *fe = find_file(fd);
	if(fe==NULL)
		return -1;

	lock_acquire(&filesys_lock);
	file_seek(fe->file, position);
	lock_release(&filesys_lock);
}

unsigned
tell(int fd)
{
	struct file_element *fe = find_file(fd);
	if(fe==NULL)
		return -1;
	lock_acquire(&filesys_lock);
	file_tell(fe->file);
	lock_release(&filesys_lock);
	return (unsigned) 1;
}

void
close(int fd)
{
	if(fd<2 || fd>=thread_current()->fd)
  	{
    	exit(-1);
  	}

	struct file_element *fe = find_file(fd);
	if(fe==NULL)
		return;

	lock_acquire(&filesys_lock);
	file_close(fe->file);
	lock_release(&filesys_lock);
	list_remove(&fe->elem);
	free(fe);
}

#ifdef VM
int
mmap(int fd, void *addr)
{
	if(fd==0 || fd==1)
		return -1;
	
	if(addr == 0 || pg_ofs(addr) !=0)
		return -1;

	if(!is_user_vaddr(addr) || addr < BOTTOM_USER_VADDR)
		return -1;

	struct file_element *fe = find_file(fd);
	if(fe==NULL) return -1;

	lock_acquire(&filesys_lock);
	struct file *f = file_reopen(fe->file);
	lock_release(&filesys_lock);
	
	if(f== NULL || file_length(f) == 0)
		return -1;
	void *iter;
	struct mmap_table_entry *mte = malloc(sizeof(struct mmap_table_entry));

	mte->file = f;
	mte->mmap_id = mmap_id;
	mte->base = addr;
	mte->size = file_length(f);
	mte->fd = fd;

	uint32_t read_bytes = file_length(f);
	int32_t ofs = 0;
	while (read_bytes > 0) 
	{
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		bool success = page_mmap(mte, page_read_bytes, page_zero_bytes, ofs, true, addr);

		if(!success)
		{
			munmap(mte->mmap_id);
			return -1;
		}

		read_bytes -= page_read_bytes;
		addr += PGSIZE;
		ofs += page_read_bytes;
	}
	list_push_back(&thread_current()->mmap_list, &mte->elem);
	mmap_id = mmap_id + 1;
	return mte->mmap_id;
}

void
munmap(int mapping)
{
	struct list_elem *e;
	struct mmap_table_entry *mte;
	for(e = list_begin(&thread_current()->mmap_list); e != list_end(&thread_current()->mmap_list); e = list_next(e))
	{
		mte = list_entry(e, struct mmap_table_entry, elem);
		if(mte->mmap_id == mapping) break;
	}
	if(mte->mmap_id != mapping)
		return;
	void *addr;

	for(addr=mte->base; addr < mte->base + mte->size ; addr += PGSIZE)
	{
		struct sup_page_table_entry *spte = find_page(addr);
		if(spte != NULL)
		{
			if(pagedir_is_dirty(thread_current()->pagedir, addr))
			{
				lock_acquire(&filesys_lock);
				int read_bytes = file_write_at(spte->file, spte->upage, spte->page_read_bytes, spte->ofs);
				lock_release(&filesys_lock);
				ASSERT(read_bytes == (int) spte->page_read_bytes);
				frame_free(addr);
			}
			list_remove(&spte->elem);
			free(spte);
		}
	}
	file_close(mte->file);
	list_remove(&mte->elem);
	free(mte);
}
#endif

#ifdef FILESYS
bool
chdir(const char *dir)
{
  lock_acquire(&filesys_lock);
  bool success = filesys_chdir(dir);
  lock_release(&filesys_lock);
  return success;
}

bool
mkdir(const char *dir)
{
  lock_acquire(&filesys_lock);
  bool success = filesys_create(dir, 0, true);
  lock_release(&filesys_lock);
  return success;
}

bool
readdir(int fd, char *name)
{
  struct file_element *fm = find_file(fd);
  if(fm==NULL)
    return false;
  if(!dir_check(fm->file))
    return false;
  if(dir_readdir(fm->file, name))
    return true;
  return false;
}

bool
isdir(int fd)
{
  struct file_element *fm = find_file(fd);
  if(fm == NULL)
    return -1;
  return dir_check(fm->file);
}

int
inumber(int fd)
{
  struct file_element *fm = find_file(fd);
  if(fm == NULL)
    return -1;
  return get_inode_number(fm->file);
}
#endif


/* Syscall_handler */
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int sys_code = get_arg((int*)f->esp);
  #ifdef VM
  thread_current()->esp = f->esp;
  #endif
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
  	#ifdef VM
  	case SYS_MMAP:
  	{
  		f->eax = mmap(get_arg((int *)f->esp+1), get_arg((int *)f->esp+2));
  		break;
  	}
  	case SYS_MUNMAP:
  	{
  		munmap(get_arg((int *)f->esp+1));
  		break;
  	}
  	#endif
  	#ifdef FILESYS

  	case SYS_CHDIR:
    {
      f->eax = chdir(get_arg((int *)f->esp+1)); 
      break;
    }
    case SYS_MKDIR:
    {
      f->eax = mkdir(get_arg((int *)f->esp+1));
      break;
    }
    case SYS_READDIR:
    {
      f->eax = readdir(get_arg((int *)f->esp+1), get_arg((int *)f->esp+2));
      break;
    }
    case SYS_ISDIR:
    {
      f->eax = isdir(get_arg((int *)f->esp+1));
      break;
    }
    case SYS_INUMBER:
    {
      f->eax = inumber(get_arg((int *)f->esp+1));
      break;
    } 
  	#endif
  }
}
