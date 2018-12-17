#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44


bool inode_allocate(struct inode_disk *, disk_sector_t);
bool inode_allocate_direct(struct inode_disk *, int );
bool inode_allocate_indirect(struct inode_disk *, int );
bool inode_allocate_double_indirect(struct inode_disk *, int);


/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}


/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static disk_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);
  if (pos >= inode->data.length)
    return -1;

  struct inode_disk *disk_inode = &inode->data;
  int index = pos / DISK_SECTOR_SIZE;

  //Direct
  if(index < DIRECT_BLOCK_NUM)
  {
    return disk_inode->direct_blocks[index];
  }
  //Indirect
  else if(index < DIRECT_BLOCK_NUM + 128)
  {
    index -= DIRECT_BLOCK_NUM;
    struct indirect_disk *disk_indirect = calloc(1, sizeof *disk_indirect);
    disk_read(filesys_disk, disk_inode->indirect_block, disk_indirect);
    disk_sector_t result = disk_indirect->sectors[index];
    free(disk_indirect);
    return result;
  }
  //Doubly indirect block
  else if (index < DIRECT_BLOCK_NUM + 128 + 128*128) 
  {
    index -= DIRECT_BLOCK_NUM + 128;
    struct indirect_disk *disk_indirect = calloc(1, sizeof *disk_indirect);
    disk_read(filesys_disk, disk_inode->doubly_indirect_block, disk_indirect);
    struct indirect_disk *disk_doubly_indirect = calloc(1, sizeof *disk_indirect);
    disk_read(filesys_disk, disk_indirect->sectors[index/128], disk_doubly_indirect);
    disk_sector_t result = disk_doubly_indirect->sectors[index%128];
    free(disk_indirect);
    free(disk_doubly_indirect);
    return result;
  }
  // File size over upper bound
  else {
    return -1;
  }
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (disk_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);
  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);

  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->is_dir = is_dir;
      disk_inode->parent = ROOT_DIR_SECTOR;
      success = true;
      size_t i;
      for(i=0; i<sectors; i++)
      {
        if(inode_allocate(disk_inode, i))
        {
          success = true;
        }
        else
        {
          success = false;
          break;
        }
      }
      if(success)
      {
        disk_write(filesys_disk, sector, disk_inode);
      }
      free (disk_inode);
    }
  return success;
}

bool 
inode_allocate(struct inode_disk *disk_inode, disk_sector_t sector)
{
  ASSERT(disk_inode != NULL);

  //direct blocks build
  if(sector < DIRECT_BLOCK_NUM)
  {
    int index = sector;
    return inode_allocate_direct(disk_inode, index);
  }
  //indirect blocks build
  else if(sector < DIRECT_BLOCK_NUM + 128)
  {
    int index = sector - DIRECT_BLOCK_NUM;
    return inode_allocate_indirect(disk_inode, index);
  }
  //doubly indirect build
  else if(sector < DIRECT_BLOCK_NUM + 128 + 128*128)
  {
    int index = sector - DIRECT_BLOCK_NUM - 128;
    return inode_allocate_double_indirect(disk_inode, index);
  }
  else
  {
    return false;
  }

}

bool 
inode_allocate_direct(struct inode_disk *disk_inode, int index)
{
  static char zeros[DISK_SECTOR_SIZE];
  if(free_map_allocate(1, &(disk_inode->direct_blocks[index])))
  {
    disk_write(filesys_disk, disk_inode->direct_blocks[index], zeros);
  }
  return true;
}

bool 
inode_allocate_indirect(struct inode_disk *disk_inode, int index)
{
  struct indirect_disk *disk_indirect = calloc(1, sizeof *disk_indirect);
  static char zeros[DISK_SECTOR_SIZE];

  if(disk_inode->indirect_block == 0)
  {
    free_map_allocate(1, &(disk_inode->indirect_block));
    memcpy(disk_indirect, zeros, DISK_SECTOR_SIZE);
  }
  else
  {
    disk_read(filesys_disk, disk_inode->indirect_block, disk_indirect);
  }

  if(free_map_allocate(1, &(disk_indirect->sectors[index])))
  {
    disk_write(filesys_disk, disk_indirect->sectors[index], zeros);
  }
  disk_write(filesys_disk, disk_inode->indirect_block, disk_indirect);
  free(disk_indirect);

  return true;
}

bool 
inode_allocate_double_indirect(struct inode_disk *disk_inode, int index)
{
  static char zeros[DISK_SECTOR_SIZE];
  struct indirect_disk *disk_indirect = calloc(1, sizeof *disk_indirect);
  if(disk_inode->doubly_indirect_block == 0)
  {
    free_map_allocate(1, &(disk_inode->doubly_indirect_block));
    memcpy(disk_indirect, zeros, DISK_SECTOR_SIZE);
  }
  else
  {
    disk_read(filesys_disk, disk_inode->doubly_indirect_block, disk_indirect);
  }

  struct indirect_disk *disk_doubly_indirect = calloc(1, sizeof *disk_doubly_indirect);
  if(disk_indirect->sectors[index/128] == 0)
  {
    free_map_allocate(1, &(disk_indirect->sectors[index/128]));
    memcpy(disk_doubly_indirect, zeros, DISK_SECTOR_SIZE);
  }
  else
  {
    disk_read(filesys_disk, disk_indirect->sectors[index/128], disk_doubly_indirect);
  }

  if(free_map_allocate(1, &(disk_doubly_indirect->sectors[index%128])))
  {
    disk_write(filesys_disk, disk_doubly_indirect->sectors[index%128], zeros);
  }
  disk_write(filesys_disk, disk_indirect->sectors[index/128], disk_doubly_indirect);
  free(disk_doubly_indirect);

  disk_write(filesys_disk, disk_inode->doubly_indirect_block, disk_indirect);
  free(disk_indirect);
  return true;
}
/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
  {
    return NULL;
  }

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  disk_read (filesys_disk, inode->sector, &inode->data);

  inode->is_dir = inode->data.is_dir;
  inode->parent = inode->data.parent;
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

void
inode_free (struct inode *inode)
{
  ASSERT (inode != NULL);

  int sectors = bytes_to_sectors(inode->data.length);
  struct inode_disk *disk_inode = &inode->data;
  int index=0;

  if(sectors==0)
  {
    return;
  }

  while(index<DIRECT_BLOCK_NUM && sectors !=0)
  {
    free_map_release(disk_inode->direct_blocks[index],1);
    sectors--;
    index++;
  }

  if(index<DIRECT_BLOCK_NUM + 128 && sectors != 0)
  {
    size_t free_blocks = sectors<128 ? sectors : 128;
    size_t i;
    disk_sector_t sector[128];
    disk_read(filesys_disk, disk_inode->indirect_block, &sector);

    for(i=0; i<free_blocks; i++)
    {
      free_map_release(sector[i], 1);
      sectors--;
      index++;
    }
    free_map_release(disk_inode->indirect_block, 1);
  }

  if(index<DIRECT_BLOCK_NUM + 128 + 128*128 && sectors != 0)
  {
    size_t i, j;
    disk_sector_t one[128], two[128];

    disk_read(filesys_disk, disk_inode->doubly_indirect_block, &one);
    for(i=0; i<sectors/128 ; i++)
    {
      size_t free_blocks = i == (sectors/128) -1 ? (sectors%128) : 128;
      disk_read(filesys_disk, one[i], &two);
      for(j=0; j<free_blocks ; j++)
      {
        free_map_release(two[j], 1);
      }
      free_map_release(one[i],1);
    }
    free_map_release(disk_inode->doubly_indirect_block, 1);
  }

}


/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
          free_map_release (inode->sector, 1);
        }
      else
        {
          inode->data.parent = inode->parent;
          inode->data.is_dir = inode->is_dir;
          disk_write(filesys_disk, inode->sector, &inode->data);
        }
      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;

  if(offset>=inode->data.length)
  {
    return bytes_read;
  }

  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          disk_read (filesys_disk, sector_idx, buffer + bytes_read);
        }
      else 
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (DISK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }
          disk_read (filesys_disk, sector_idx, bounce);
          memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
        }
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  free (bounce);
  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
  {
    return 0;
  }

  if(inode->data.length < offset+size)
  {
    size_t i;
    size_t length = bytes_to_sectors(inode->data.length);
    size_t add_length = bytes_to_sectors(offset+size);
    for(i= length ; i<add_length ; i++)
    {
      inode_allocate(&inode->data, i);
    }
    inode->data.length = offset+size;
    disk_write(filesys_disk, inode->sector, &inode->data);
  }

  while (size > 0) 
    {
      /* Sector to write, starting byte offset within sector. */
      disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE)
        {
          /* Write full sector directly to disk. */
          disk_write (filesys_disk, sector_idx, buffer + bytes_written);
        }
      else 
        {
          /* We need a bounce buffer. */
          if (bounce == NULL) 
            {
              bounce = malloc (DISK_SECTOR_SIZE);
              if (bounce == NULL)
                break;
            }

          /* If the sector contains data before or after the chunk
             we're writing, then we need to read in the sector
             first.  Otherwise we start with a sector of all zeros. */
          if (sector_ofs > 0 || chunk_size < sector_left) 
            disk_read (filesys_disk, sector_idx, bounce);
          else
            memset (bounce, 0, DISK_SECTOR_SIZE);
          memcpy (bounce + sector_ofs, buffer + bytes_written, chunk_size);
          disk_write (filesys_disk, sector_idx, bounce);
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
  free (bounce);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
