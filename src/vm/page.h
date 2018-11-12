#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "lib/kernel/hash.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "filesys/off_t.h"

enum page_type
{
	PAGE_STACK,
	PAGE_FILE,
	PAGE_SWAP,
};

struct sup_page_table_entry
{
	void *upage;
	void *kpage;
	int type; 
	bool writable;

	struct file *file;
	off_t ofs;
	size_t page_read_bytes;
	size_t page_zero_bytes;

	int swap_index;
	struct list_elem elem;

};

void page_grow_stack(void *addr);
struct sup_page_table_entry* page_file(struct file *file, off_t ofs, uint8_t *upage, size_t page_read_bytes, size_t page_zero_bytes, bool writable);
bool page_load(void *addr);
struct sup_page_table_entry* find_page(void *addr);
bool page_load_file(struct sup_page_table_entry *spte);
bool page_swap_in(struct sup_page_table_entry *spte);
#endif