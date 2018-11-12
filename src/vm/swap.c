#include "vm/swap.h"

#define SWAP_FREE 0
#define SWAP_IN_USE 1

void 
swap_init(void)
{
	swap_disk = disk_get(1,1);
	swap_bitmap = bitmap_create(disk_size(swap_disk));
	bitmap_set_all(swap_bitmap, SWAP_FREE);
	lock_init(&swap_lock);
}

size_t 
swap_out (void *frame)
{
  // if (!swap_disk || !swap_bitmap)
  // {
  //     PANIC("Need swap partition but no swap partition present!");
  // }
  lock_acquire(&swap_lock);

  size_t free_index = bitmap_scan_and_flip(swap_bitmap, 0, 1, SWAP_FREE);
  
  size_t i;
  for (i = 0; i < SECTORS_PER_PAGE; i++)
  { 
      disk_write(swap_disk, free_index * SECTORS_PER_PAGE + i, (uint8_t *) frame + i * DISK_SECTOR_SIZE);
  }
  lock_release(&swap_lock);
  return free_index;
}

void 
swap_in (size_t used_index, void* frame)
{
  if (!swap_disk || !swap_bitmap)
  {
      return;
  }
  lock_acquire(&swap_lock);
  // if (bitmap_test(swap_bitmap, used_index) == SWAP_FREE)
  // {
  //     PANIC ("Trying to swap in a free block! Kernel panicking.");
  // }
  bitmap_flip(swap_bitmap, used_index);
  size_t i;
  for (i = 0; i < SECTORS_PER_PAGE; i++)
  {
      disk_read(swap_disk, used_index * SECTORS_PER_PAGE + i, (uint8_t *) frame + i * DISK_SECTOR_SIZE);
  }
  lock_release(&swap_lock);
}