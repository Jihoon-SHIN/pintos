#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"
#include <list.h>

#define DIRECT_BLOCK_NUM 122

struct bitmap;

/* Disk used for file system. */
extern struct disk *filesys_disk;

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk
{
    disk_sector_t direct_blocks[DIRECT_BLOCK_NUM];   /* Direct blocks */
    disk_sector_t indirect_block;                    /* Indirect blocks */
    disk_sector_t doubly_indirect_block;             /* Doubly indirect blocks */
    off_t length;                                     /* File size in bytes. */
    unsigned magic;                                   /* Magic number. */
    bool is_dir;                                      /* To check whether directory or not */
    disk_sector_t parent;                            /* Store the parent disk_sector_t */
};

struct indirect_disk
{
  disk_sector_t sectors[128];
};

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    bool is_dir;                        /* To check whether directory or not */
    disk_sector_t parent;              /* Store the parent */
  };


void inode_init (void);
bool inode_create (disk_sector_t, off_t, bool);
struct inode *inode_open (disk_sector_t);
struct inode *inode_reopen (struct inode *);
disk_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);

#endif /* filesys/inode.h */