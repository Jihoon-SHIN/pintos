#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "devices/disk.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <bitmap.h>


#define SECTORS_PER_PAGE (PGSIZE / DISK_SECTOR_SIZE)

// struct lock swap_lock;
// struct bitmap* swap_bitmap;
// struct block* swap_block;


// void swap_init(void);
// size_t swap_out(void *frame);
// void swap_in(size_t index, void * frame);

#endif