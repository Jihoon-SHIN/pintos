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
};

struct sup_page_table_entry
{
	void *upage;
	int type; 
	bool writable;

	struct file *file;
	off_t ofs;
	size_t page_read_bytes;
	size_t page_zero_bytes;

	struct list_elem elem;

};

void page_grow_stack(void *addr);
void page_file(struct file *file, off_t ofs, uint8_t *upage, size_t page_read_bytes, size_t page_zero_bytes, bool writable);

#endif