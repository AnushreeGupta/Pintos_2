#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

static void      sys_halt (void);
static void      sys_exit (int status);
static pid_t     sys_exec (const char *file);
static int       sys_wait (pid_t pid);
static bool      sys_create (const char *file, unsigned initial_size);
static bool      sys_remove (const char *file);
static int       sys_open (const char *file);
static int       sys_filesize (int fd);
static int       sys_read (int fd, void *buffer, unsigned size);
static int       sys_write (int fd, const void *buffer, unsigned size);
static void      sys_seek (int fd, unsigned position);
static unsigned  sys_tell (int fd);
static void      sys_close (int fd);
static mapid_t   sys_mmap (int fd, void *addr);
static void      sys_munmap (mapid_t mapid);

struct user_file
  {
    struct file *file;                 /* Pointer to the actual file */
    fid_t fid;                         /* File identifier */
    struct list_elem thread_elem;      /* List elem for a thread's file list */
};

static struct lock file_lock;
static struct list file_list;
struct file* get_file (int fd);
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  syscall_map[SYS_HALT]     = (handler)sys_halt;
  syscall_map[SYS_EXIT]     = (handler)sys_exit;
  syscall_map[SYS_EXEC]     = (handler)sys_exec;
  syscall_map[SYS_WAIT]     = (handler)sys_wait;
  syscall_map[SYS_CREATE]   = (handler)sys_create;
  syscall_map[SYS_REMOVE]   = (handler)sys_remove;
  syscall_map[SYS_OPEN]     = (handler)sys_open;
  syscall_map[SYS_FILESIZE] = (handler)sys_filesize;
  syscall_map[SYS_READ]     = (handler)sys_read;
  syscall_map[SYS_WRITE]    = (handler)sys_write;
  syscall_map[SYS_SEEK]     = (handler)sys_seek;
  syscall_map[SYS_TELL]     = (handler)sys_tell;
  syscall_map[SYS_CLOSE]    = (handler)sys_close;
  syscall_map[SYS_MMAP]     = (handler)sys_mmap;
  syscall_map[SYS_MUNMAP]   = (handler)sys_munmap;

  lock_init (&file_lock);
  list_init (&file_list);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

static void
sys_halt (void)
{
  shutdown_power_off ();
}

static void
sys_exit (int status)
{
  struct thread *cur;
  struct list_elem *temp;

  cur = thread_current ();
  if (lock_held_by_current_thread (&file_lock) )
    lock_release (&file_lock);

  /* Close all opened files of the thread. */
  while (!list_empty (&cur->files_lsit) )
    {
      temp = list_begin (&cur->files_list);
      sys_close (list_entry (temp, struct user_file, thread_elem)->fid );
    }
  /* Unmap all memory mapped files of the thread. */
  while (!list_empty (&cur->mfiles) )
    {
      temp = list_begin (&cur->mfiles);
      sys_munmap ( list_entry (temp, struct vm_mfile, thread_elem)->mapid );
    }

  cur->ret_status = status;
  //displaying the exit msg for all threads
  printf ("%s: exit(%d)\n", cur->name, status);
  thread_exit ();
}

static pid_t
sys_exec (const char *file)
{
  lock_acquire (&file_lock);
  int ret = process_execute (file);
  lock_release (&file_lock);
  return ret;
}

static int
sys_wait (pid_t pid)
{
  return process_wait (pid);
}

static bool
sys_create (const char *file, unsigned initial_size)
{
  if (file == NULL)
    sys_exit (-1);

  lock_acquire (&file_lock);
  bool ret = filesys_create (file, initial_size);
  lock_release (&file_lock);
  return ret;
}

bool sys_remove (const char *file)
{
if (file == NULL)
    sys_exit (-1);
  lock_acquire(&file_lock);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  return success;
}

static int
sys_open (const char *file)
{
  if (file == NULL)
    return -1;
   lock_acquire (&file_lock);
  struct file *sys_file  = filesys_open (file);
  struct user_file *f;

  lock_release (&file_lock);
  if (sys_file == NULL)
    return -1;

  f = (struct user_file *) malloc (sizeof (struct user_file));
  if (f == NULL)
    {
      file_close (sys_file);
      return -1;
    }

  lock_acquire (&file_lock);
  f->file = sys_file;
  f->fid = allocate_fid ();
  list_push_back (&thread_current ()->files, &f->thread_elem);
  lock_release (&file_lock);

  return f->fid;
}

static int
sys_filesize (int fd)
{
  lock_acquire(&filesys_lock);
  struct file *f = process_get_file(fd);
  int size = -1;
  if (!f)
    {
      lock_release(&filesys_lock);
      return ERROR;
    }
  size = file_length(f);
  lock_release(&filesys_lock);
return size;
}

static int
sys_read (int fd, void *buffer, unsigned length)
{
}


static int
sys_write (int fd, const void *buffer, unsigned length)
{
}


static void
sys_seek (int fd, unsigned position)
{
  struct user_file *f;

  f = file_by_fid (fd);
  if (!f)
    sys_exit (-1);

  lock_acquire (&file_lock);
  file_seek (f->file, position);
  lock_release (&file_lock);
}


static unsigned
sys_tell (int fd)
{

}


static void
sys_close (int fd)
{

}


mapid_t
sys_mmap (int fd, void *addr)
{

}

void
sys_munmap (mapid_t mapid)
{

}


