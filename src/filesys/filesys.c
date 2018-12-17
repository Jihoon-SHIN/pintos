#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

/* Partition that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);
struct dir * parse_dir(const char *name);
char * parse_filename(const char *name);


/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size, bool is_dir) 
{
  disk_sector_t inode_sector = 0;
  bool success = false;
  char *file_name = parse_filename(name);
  struct dir *dir= parse_dir(name);

  if(strcmp(file_name, ".")!=0 && strcmp(file_name, "..")!=0)
  {
     success = (dir != NULL
                    && free_map_allocate (1, &inode_sector)
                    && inode_create (inode_sector, initial_size, is_dir)
                    && dir_add (dir, file_name, inode_sector));
  } 
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  if(strlen(name) == 0)
    return NULL;

  struct inode *inode = NULL;
  char *file_name = parse_filename(name);
  struct dir *dir = parse_dir(name);

  if (dir != NULL)
  {
    if(strcmp(file_name, ".") == 0 || (dir->inode->sector==ROOT_DIR_SECTOR) && (strlen(file_name) == 0))
    {
      free(file_name);
      return (struct file *)dir;
    }
    else if(strcmp(file_name, "..")==0)
    {
      inode = dir_parent_inode(dir);
      if(inode == NULL)
      {
        free(file_name);
        return NULL;
      }
    }
    else
      dir_lookup (dir, file_name, &inode);
  }
  free(file_name);
  dir_close (dir);

  if(inode ==NULL)
    return NULL;

  if(inode->is_dir)
    return (struct file *) dir_open(inode);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = parse_dir(name);
  char *file_name = parse_filename(name);
  bool success = dir != NULL && dir_remove (dir, file_name);
  dir_close (dir);
  free(file_name); 
  return success;
}

bool
filesys_chdir (const char *name)
{
  struct dir *dir = parse_dir(name);
  // struct dir *new_dir;
  struct inode *inode;
  char *file_name = parse_filename(name);
  if(dir != NULL)
  {
    if(strcmp(file_name, ".") == 0)
    {
      thread_current()->working_dir = dir;
      free(file_name);
      return true;
    }
    else if(dir->inode->sector==ROOT_DIR_SECTOR && strlen(file_name) == 0)
    {
      thread_current()->working_dir = dir;
      free(file_name);
      return true;
    }
    else if(strcmp(file_name, "..")==0)
    {
      inode = dir_parent_inode(dir);
      if(inode == NULL)
      {
        free(file_name);
        return false;
      }
    }
    else
      dir_lookup (dir, file_name, &inode);
    
    dir_close(dir);
    dir = dir_open(inode);
    if(dir == NULL)
    {
      free(file_name);
      return false;
    }
    dir_close(thread_current()->working_dir);
    thread_current()->working_dir = dir;
    free(file_name);
    return true;
  }
  free(file_name);
  return false;
}




/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/* Additional implemention */
char *
parse_filename(const char *name)
{
  int name_length = strlen(name);
  char str[name_length+1];
  memcpy(str, name, name_length+1);
  char *token, *save_ptr, *prev_token = "";

  for(token = strtok_r(str, "/", &save_ptr) ; token != NULL ; token = strtok_r(NULL, "/", &save_ptr))
  {
    prev_token = token;
  }
  char *file_name = malloc(strlen(prev_token)+1);
  memcpy(file_name, prev_token, strlen(prev_token)+1);
  return file_name;
}

struct dir *
parse_dir(const char *name)
{
  int name_length = strlen(name);
  char path[name_length+1];
  memcpy(path, name, name_length+1);

  char *cur_token, *save_ptr, *prev_token;
  struct dir *dir;
  if(path[0]=='/' || thread_current()->working_dir == NULL)
  {
    dir = dir_open_root();
  }
  else
  {
    dir = dir_reopen(thread_current()->working_dir);
  }

  prev_token = strtok_r(path, "/", &save_ptr);
  for(cur_token = strtok_r(NULL, "/", &save_ptr) ; cur_token != NULL; cur_token = strtok_r(NULL, "/", &save_ptr))
  {
    struct inode *inode;
    if(strcmp(prev_token, ".") == 0)
    {
      continue;
    }
    else if(strcmp(prev_token, "..") ==0)
    {
      inode = dir_parent_inode(dir);
      if(inode == NULL) 
        return NULL;
    }
    else if(dir_lookup(dir, prev_token, &inode) == false)
    {
      return NULL;
    }
    if(inode->is_dir)
    {
      dir_close(dir);
      dir = dir_open(inode);
    }
    else
    {
      inode_close(inode);
    }
    prev_token = cur_token;
  }
  return dir;
}