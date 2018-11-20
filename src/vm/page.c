#include <string.h>
#include <stdbool.h>
#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include "filesys/off_t.h"



void
page_grow_stack(void *addr)
{
	struct sup_page_table_entry *spte = malloc(sizeof(struct sup_page_table_entry));
	bool success = false;
	spte->upage = pg_round_down(addr);
	spte->type = PAGE_STACK;
	spte->writable = true;

	// uint8_t *kpage = frame_allocate(PAL_USER | PAL_ZERO, spte);

	// if (kpage != NULL) 
	// {
 //      success = install_page (spte->upage, kpage, true);
 //      if(!success)
 //      {
 //      	free(spte);
 //        frame_free(kpage);
 //      }
	// }

	list_push_back(&thread_current()->spt, &spte->elem);
}

struct sup_page_table_entry *
page_file(struct file *file, off_t ofs, uint8_t *upage, size_t page_read_bytes, size_t page_zero_bytes, bool writable)
{
	struct sup_page_table_entry *spte = malloc(sizeof(struct sup_page_table_entry));

	spte->upage = upage;
	spte->file = file;
	spte->ofs = ofs;
	spte->page_read_bytes = page_read_bytes;
	spte->page_zero_bytes = page_zero_bytes;
	spte->writable = writable;
	spte->type = PAGE_FILE;
	// spte->type = PAGE_LOADED;

	// if(find_page2(spte->upage))
	// {
	// 	free(spte);
	// 	return NULL;
	// }

	list_push_back(&thread_current()->spt, &spte->elem);
	return spte;
}

bool
page_mmap(struct mmap_table_entry * mte, size_t page_read_bytes, size_t page_zero_bytes, off_t ofs, bool writable, uint8_t *upage)
{
	struct sup_page_table_entry *spte = malloc(sizeof(struct sup_page_table_entry));

	spte->upage = upage;
	spte->file = mte->file;
	spte->mte = mte;
	spte->type = PAGE_MMAP;
	spte->writable = writable;
	spte->page_read_bytes = page_read_bytes;
	spte->page_zero_bytes = page_zero_bytes;
	spte->ofs = ofs;

	if(find_page2(spte->upage))
	{
		free(spte);
		return false;
	}

	list_push_back(&thread_current()->spt, &spte->elem);
	return true;
}


bool
page_load(void *addr)
{
	struct sup_page_table_entry *spte = find_page(addr);
	if(spte == NULL)
	{
		return false;
	}
	switch(spte->type)
	{
		case PAGE_SWAP:
			return page_swap_in(spte);
		case PAGE_FILE:
			return page_load_file(spte);
		case PAGE_STACK:
			return page_load_stack(spte);
		case PAGE_MMAP:
			return page_load_mmap(spte);
		default:
			return false;

	}
}


struct sup_page_table_entry*
find_page(void *addr)
{
	struct list_elem *e;
	uint8_t *addr_d = pg_round_down(addr);	
	for(e=list_begin(&thread_current()->spt); e != list_end(&thread_current()->spt) ; e = list_next(e))
	{
		struct sup_page_table_entry * spte = list_entry(e, struct sup_page_table_entry, elem);
		if(spte->upage == addr_d)
			return spte;
	}
	return NULL;
}

bool
find_page2(void *addr)
{
	struct list_elem *e;
	uint8_t *addr_d = pg_round_down(addr);	
	for(e=list_begin(&thread_current()->spt); e != list_end(&thread_current()->spt) ; e = list_next(e))
	{
		struct sup_page_table_entry * spte = list_entry(e, struct sup_page_table_entry, elem);
		if(spte->upage == addr_d)
			return true;
	}
	return false;
}

bool
page_load_file(struct sup_page_table_entry *spte) 
{  
  uint8_t *kpage = frame_allocate(PAL_USER | PAL_ZERO, spte);
  bool success = true;
  /* Load this page. */
  // lock_acquire(&filesys_lock);
  if (file_read_at (spte->file, kpage, spte->page_read_bytes, spte->ofs) != (int) spte->page_read_bytes)
  	ASSERT(0);

  // lock_release(&filesys_lock);


  // if(spte->page_read_bytes > 0)
  // memset (kpage + spte->page_read_bytes, 0, spte->page_zero_bytes);
  spte->type = PAGE_LOADED;

  if (kpage != NULL) 
  {
  	success = install_page (spte->upage, kpage, spte->writable);
  	if(!success)
  	{
  		free(spte);
  		frame_free(kpage);
  		return success;
  	}
  }
  return success;
}


bool
page_swap_in(struct sup_page_table_entry *spte)
{
	uint8_t *kpage = frame_allocate(PAL_USER, spte);
	if(kpage == NULL)
	{
		return false;
	}
	
	if(!install_page(spte->upage, kpage, spte->writable))
	{
		frame_free(kpage);
		return false;
	}
	swap_in(spte->swap_index, kpage);
	spte->type = PAGE_LOADED;
	return true;
}

bool
page_load_stack(struct sup_page_table_entry *spte)
{
	uint8_t *kpage = frame_allocate(PAL_USER|PAL_ZERO, spte);
	bool success = true;
	if (kpage != NULL) 
	{
      success = install_page (spte->upage, kpage, spte->writable);
      if(!success)
      {
      	free(spte);
        frame_free(kpage);
        return success;
      }
	}
	spte->type = PAGE_LOADED;
	return success;
}


bool
page_load_mmap(struct sup_page_table_entry *spte)
{
	uint8_t *kpage = frame_allocate(PAL_USER | PAL_ZERO, spte);
	bool success = true;
	/* Load this page. */
	// lock_acquire(&filesys_lock);
	if (file_read_at (spte->file, kpage, spte->page_read_bytes, spte->ofs) != (int) spte->page_read_bytes)
		ASSERT(0);

	if (kpage != NULL) 
	{
		success = install_page (spte->upage, kpage, spte->writable);
		if(!success)
		{
			free(spte);
			frame_free(kpage);
			return success;
		}
	}
	spte->type = PAGE_LOADED;
	return success;
}