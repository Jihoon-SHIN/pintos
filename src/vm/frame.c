#include <stdlib.h>
#include "filesys/file.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"


static struct list frame_table_list;
static struct lock frame_table_lock;

void
frame_init(void)
{
	list_init(&frame_table_list);
	lock_init(&frame_table_lock);
}

uint8_t *
frame_allocate(enum palloc_flags flags, struct sup_page_table_entry *spte)
{	
	uint8_t *kpage;
	struct frame_table_entry *fte = malloc(sizeof(struct frame_table_entry));


	kpage = palloc_get_page(flags);

	if(kpage == NULL)
	{
		evict_frame();
		kpage = palloc_get_page(flags);
		ASSERT(kpage != NULL);
	}
	fte->frame = kpage;
	fte->thread = thread_current();
	fte->spte = spte;
	
	lock_acquire(&frame_table_lock);	
	list_push_back(&frame_table_list, &fte->elem);
	lock_release(&frame_table_lock);
	return kpage;
}

void
frame_free(void *frame)
{
	lock_acquire(&frame_table_lock);
	struct frame_table_entry *fte = frame_find(frame);
	if(fte != NULL)
	{
		list_remove(&fte->elem);
		palloc_free_page(frame);
		free(fte);
	}
	lock_release(&frame_table_lock);
}

struct frame_table_entry *
frame_find(void *frame)
{
	struct list_elem* e;
	struct frame_table_entry *fte;
	// lock_acquire(&frame_table_lock);
	for(e= list_begin(&frame_table_list) ; e != list_end(&frame_table_list) ; e = list_next(e))
	{
		fte = list_entry(e, struct frame_table_entry, elem);
		if(fte->frame == frame)
		{
			// lock_release(&frame_table_lock);
			return fte;
		}
	}
	// lock_release(&frame_table_lock);
	return NULL;
}

void
evict_frame(void)
{
	lock_acquire(&frame_table_lock);

	struct list_elem *e;
	struct frame_table_entry *fte;
	for(e=list_begin(&frame_table_list) ; e!=list_end(&frame_table_list) ; e = list_next(e))
	{
		fte = list_entry(e, struct frame_table_entry, elem);
		if(fte->spte->type !=PAGE_STACK)
		{
		// if(pagedir_is_accessed(fte->thread->pagedir, fte->frame))
		// {
		// 	pagedir_set_accessed(fte->thread->pagedir, fte->frame, false);
		// }
		// else
		// {
			fte->spte->swap_index = swap_out(fte->frame);
			fte->spte->type = PAGE_SWAP;
			list_remove(&fte->elem);
			pagedir_clear_page(fte->thread->pagedir, fte->spte->upage);
			palloc_free_page(fte->frame);
			free(fte);
			break;
		}
		// }		
	}
	lock_release(&frame_table_lock);
}


