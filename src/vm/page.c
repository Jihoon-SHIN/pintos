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

	uint8_t *kpage = frame_allocate(PAL_USER | PAL_ZERO);

	// printf("hello\n");
	if (kpage != NULL) 
	{
      success = install_page (spte->upage, kpage, true);
      // ASSERT(success);
      // if (success)
      // {
      //   *esp = PHYS_BASE;
      // }
      if(!success)
      {
        frame_free(kpage);
      }
	}
}

void
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
	list_push_back(&thread_current()->spt, &spte->elem);
}




// bool 
// page_load_file(struct sup_page_table_entry *spte) 
// {  
//   uint8_t *kpage = frame_allocate(PAL_USER);
//   spte->upage = kpage;

//   /* Load this page. */
//   if (file_read_at (page->file, kpage, page->page_read_bytes, page->ofs) != (int) page->page_read_bytes)
//     ASSERT(0);
//   memset (kpage + page->page_read_bytes, 0, page->page_zero_bytes);

//   page->type = PAGE_LOADED;

//   return true;
// }

