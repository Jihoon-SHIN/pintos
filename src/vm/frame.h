#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "vm/page.h"
#include <stdbool.h>
#include <stdint.h>
#include <list.h>



struct frame_table_entry
{
	void* frame;
	struct thread *thread;
	struct sup_page_table_entry *spte;
	// int count_LRU;
	struct list_elem elem;
};

uint8_t *frame_allocate(enum palloc_flags flags, struct sup_page_table_entry * spte);
struct frame_table_entry * frame_find(void *frame);
void frame_free(void *frame);
void evict_frame(void);
#endif