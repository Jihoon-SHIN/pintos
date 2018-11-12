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
frame_allocate(enum palloc_flags flags)
{	
	uint8_t *kpage;
	struct frame_table_entry *fte = malloc(sizeof(struct frame_table_entry));

	kpage = palloc_get_page(flags);

	if(kpage == NULL)
	{
		PANIC("We need to SWAP");
	}

	fte->frame = kpage;
	fte->thread = thread_current();

	list_push_back(&frame_table_list, &fte->elem);
	return kpage;
}

void
frame_free(void *frame)
{
	// lock_acquire(&frame_table_lock);
	struct frame_table_entry *fte = frame_find(frame);
	if(fte != NULL)
	{
		list_remove(&fte->elem);
		palloc_free_page(frame);
		free(fte);
	}
	// lock_release(&frame_table_lock);
}

struct frame_table_entry *
frame_find(void *frame)
{
	struct list_elem* e;
	struct frame_table_entry *fte;
	for(e= list_begin(&frame_table_list) ; e != list_end(&frame_table_list) ; e = list_next(e))
	{
		fte = list_entry(e, struct frame_table_entry, elem);
		if(fte->frame == frame)
		{
			return fte;
		}
	}
	return NULL;
}
