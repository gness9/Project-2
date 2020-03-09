#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "threads/init.h"
#include "devices/shutdown.h" /* Imports shutdown_power_off() for use in halt(). */
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "threads/malloc.h"

static void syscall_handler (struct intr_frame *);
struct entry_file * obtain_file(int fd);
void * obtain_arguments(const void *vaddr);

/* Get up to three arguments from a programs stack (they directly follow the system
call argument). */
void get_stack_arguments (struct intr_frame *f, int * args, int num_of_args);

/* Creates a struct to insert files and their respective file descriptor into
   the file_descriptors list for the current thread. */
struct thread_file
{
    struct list_elem file_elem;
    struct file *file_addr;
    int file_descriptor;
};

/* Lock is in charge of ensuring that only one process can access the file system at one time. */
struct lock lock_filesys;

void
syscall_init (void)
{
  /* Initialize the lock for the file system. */
  lock_init(&lock_filesys);

  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


/* Handles a system call initiated by a user program. */
static void
syscall_handler (struct intr_frame *f UNUSED)
{
	void * virt_page;
	void * stack_pointer = f->esp;
	validate_address((const void *)stack_pointer);

	int args[3];

	switch(*(int *)stack_pointer)
	{
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			exit(arg);
			break;
		case SYS_EXEC: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			arg = *(int *)stack_pointer;
			virt_page = (void *) pagedir_get_page(thread_current()->pagedir, (const void *) arg);
			if (virt_page == NULL) {
				exit(-1);
			}
			arg = (int) virt_page;
			f->eax = exec((const char *) arg);
			break;
		case SYS_WAIT: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			arg = *(int *)stack_pointer;
			f->eax = wait((pid_t) arg);
			break;
		case SYS_CREATE: ;
			for (int i = 0; i < 2; i++) {
				stack_pointer = (int *) f->esp + i + 1;
				validate_address((const void *) stack_pointer);
				args[i] = *(int *)stack_pointer;
			}
			char *stack_pointer = (char * )args[0];
			for (int i = 0; i < args[1]; i++) {
				validate_address((const void *) stack_pointer);
				stack_pointer++;
			}
			virt_page = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
			if (virt_page == NULL) {
				exit(-1);
			}
			args[0] = (int) virt_page;
			f->eax = create((const char *) args[0], (unsigned) args[1]);
			break;
		case SYS_REMOVE: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			arg = *(int *)stack_pointer;
			virt_page = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
			if (virt_page == NULL) {
				exit(-1);
			}
			arg = (int) virt_page;
			f->eax = remove((const char *) arg);
			break;
		case SYS_OPEN: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			arg = *(int *)stack_pointer;
			virt_page = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
			if (virt_page == NULL) {
				exit(-1);
			}
			arg = (int) virt_page;
			f->eax = open((const char *) arg);
			break;
		case SYS_FILESIZE: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			arg = *(int *)stack_pointer;
			f->eax = filesize(arg);
			break;
		case SYS_READ: ;
			for (int i = 0; i < 3; i++) {
				stack_pointer = (int *) f->esp + i + 1;
				validate_address((const void *) stack_pointer);
				args[i] = *(int *)stack_pointer;
			}
			char *stack_pointer = (char * )args[1];
			for (int i = 0; i < args[2]; i++) {
				validate_address((const void *) stack_pointer);
				stack_pointer++;
			}
			virt_page = pagedir_get_page(thread_current()->pagedir, (const void *) args[1]);
			if (virt_page == NULL) {
				exit(-1);
			}
			args[1] = (int) virt_page;
			f->eax = read(args[0], (void *) args[1], (unsigned) args[2]);
			break;
		case SYS_WRITE: ;
			for (int i = 0; i < 3; i++) {
				stack_pointer = (int *) f->esp + i + 1;
				validate_address((const void *) stack_pointer);
				args[i] = *(int *)stack_pointer;
			}
			char *stack_pointer = (char * )args[1];
			for (int i = 0; i < args[2]; i++) {
				validate_address((const void *) stack_pointer);
				stack_pointer++;
			}
			virt_page = pagedir_get_page(thread_current()->pagedir, (const void *) args[1]);
			if (virt_page == NULL) {
				exit(-1);
			}
			args[1] = (int) virt_page;
			f->eax = write(args[0], (const void *) args[1], (unsigned) args[2]);
			break;
		case SYS_SEEK: ;
			for (int i = 0; i < 2; i++) {
				stack_pointer = (int *) f->esp + i + 1;
				validate_address((const void *) stack_pointer);
				args[i] = *(int *)stack_pointer;
			}
			seek(args[0], (unsigned) args[1]);
			break;
		case SYS_TELL: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			arg = *(int *)stack_pointer;
			f->eax = tell(arg);
			break;
		case SYS_CLOSE: ;
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			arg = *(int *)stack_pointer;
			close(args);
			break;
		default:
			exit(-1);
			break;
	}
}

/*
void check_buffer (void *buff_to_check, unsigned size)
{
	unsigned i;
	char *stack_pointer  = (char * )buff_to_check;
	for (i = 0; i < size; i++)
	{
		validate_address((const void *) stack_pointer);
		stack_pointer++;
	}
}
*/

/*
void get_stack_arguments (struct intr_frame *f, int *args, int num_of_args)
{
  	int i;
  	int *stack_pointer;
  	for (i = 0; i < num_of_args; i++)
	{
		stack_pointer = (int *) f->esp + i + 1;
		validate_address((const void *) stack_pointer);
		args[i] = *stack_pointer;
	}
}
*/

/*Implement  the  system  call  handler  in userprog/syscall.c.  
The  skeleton  implementation  we  provide  "handles" system  calls  by  terminating  the  process.  
It  will  need  to  retrieve  the  system  call  number,  
then  any  system  call arguments, and carry out appropriate actions.*/
/* static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	
  validate_address((const void *) f->esp);

  int * p = f->esp; 

  int system_call = * p;  
	
  if (f->esp == NULL)
  {
    exit(-1);
  }
  
  printf("\nCCCCCCC\n");
  printf("BLARGH: %d, %d", *((int*)f->esp+1), SYS_WRITE);
  //int * args = f->esp;
  
  switch(*((int*)f->esp+1)) 
  {
    case SYS_HALT:
	  halt();
      break;
    case SYS_EXIT: ;
	  printf("\nTEST");
	  int status = *((int*)f->esp+1);
	  printf("\nstatus: %d", status);
	  exit(status);
      break;
    case SYS_EXEC: ;
	  char * cmd_line = (char*)(*((int*)f->esp+1));
	  f->eax = exec(cmd_line);
      break;
    case SYS_WAIT: ;
	  pid_t pid = *((pid_t*)f->esp+1);
	  f->eax = wait(pid);
      break;
    case SYS_CREATE: ;
	  char * file_create = (char*)(*((int*)f->esp+1));
	  unsigned initial_size = *((unsigned*)f->esp+2);
	  f->eax = create(file_create, initial_size);
      break;
    case SYS_REMOVE: ;
	  char * file_remove = (char*)(*((int*)f->esp+1));
	  f->eax = remove(file_remove);
      break;
    case SYS_OPEN: ;
	  //char * file_open = (char*)(*((int*)f->esp+1));
	  f->eax = open(file_open);
      break;
    case SYS_FILESIZE: ;
	  int fd_fs = *((int*)f->esp+1);
	  f->eax = filesize(fd_fs);
      break;
    case SYS_READ: ;
	  int fd_r = *((int*)f->esp+1);
	  void * buffer_r = (void*)(*((int*)f->esp+2));
      unsigned size_r = *((unsigned*)f->esp+3);
      f->eax = read(fd_r, buffer_r, size_r);
	  break;
    case SYS_WRITE: ;
	  int fd_w = *((int*)f->esp+1);
	  void * buffer_w = (void*)(*((int*)f->esp+2));
      unsigned size_w = *((unsigned*)f->esp+3);
	  f->eax = write(fd_w, buffer_w, size_w);
      break;
    case SYS_SEEK: ;
	  int fd_s = *((int*)f->esp+1);
	  unsigned position = *((unsigned*)f->esp+2);
	  seek(fd_s, position);
      break;
    case SYS_TELL: ;
	  int fd_t = *((int*)f->esp+1);
	  tell(fd_t);
      break;
    case SYS_CLOSE: ;
	  int fd_c = *((int*)f->esp+1);
	  close(fd_c);
      break;
    default:
		printf("FAILED ALL");
  }
  thread_exit ();
} */

void validate_address (const void *stack_pointer)
{
	if(!is_user_vaddr(stack_pointer) || stack_pointer == NULL 
		|| stack_pointer < (void *) 0x08048000)
	{
		exit(-1);
	}
}

/*Terminates Pintos by calling shutdown_power_off()*/
void halt (void)
{
	shutdown_power_off();
}

void exit(int status) 
{
	thread_current()->status_exit = status;
	printf("%s: exit(%d)\n", thread_current()->name, status);
	thread_exit ();
}

pid_t exec (const char * file)
{
	if(!file)
	{
		return -1;
	}
	lock_acquire(&lock_filesys);
	pid_t child_tid = process_execute(file);
	lock_release(&lock_filesys);
	return child_tid;
}


/*Waits for a child process pidand retrieves the child's exit status. 
If pidis still alive, waits until it terminates. Then, returns the  status that pidpassed to exit.*/

int wait(pid_t pid)
{
	return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
	lock_acquire(&lock_filesys);
	bool file_create = filesys_create(file, initial_size);
	lock_release(&lock_filesys);
	return file_create;
} 

bool remove (const char *file) 
{
	lock_acquire(&lock_filesys);
	bool file_remove = filesys_remove(file);
	lock_release(&lock_filesys);
	return file_remove;
}

int open(const char *file) 
{
	lock_acquire(&lock_filesys);
	/* Semaphore/lock should go here */
	struct file* openedFile = filesys_open(file);
	if (openedFile == NULL)
	{
		lock_release(&lock_filesys);
		// Release lock
		return -1;
	}

  /* Create a struct to hold the file/fd, for use in a list in the current process.
     Increment the fd for future files. Release our lock and return the fd as an int. */
  struct thread_file *new_file = malloc(sizeof(struct thread_file));
  new_file->file_addr = file;
  int fd = thread_current ()->cur_fd;
  thread_current ()->cur_fd++;
  new_file->file_descriptor = fd;
  list_push_front(&thread_current ()->file_descriptors, &new_file->file_elem);
  lock_release(&lock_filesys);
  return fd;
}


/* Returns the size, in bytes, of the file open as fd. */
int filesize (int fd)
{
  /* list element to iterate the list of file descriptors. */
  struct list_elem *temp;

  lock_acquire(&lock_filesys);

  /* If there are no files associated with this thread, return -1 */
  if (list_empty(&thread_current()->file_descriptors))
  {
    lock_release(&lock_filesys);
    return -1;
  }

  /* Check to see if the given fd is open and owned by the current process. If so, return
     the length of the file. */
  for (temp = list_front(&thread_current()->file_descriptors); temp != NULL; temp = temp->next)
  {
      struct thread_file *t = list_entry (temp, struct thread_file, file_elem);
      if (t->file_descriptor == fd)
      {
        lock_release(&lock_filesys);
        return (int) file_length(t->file_addr);
      }
  }

  lock_release(&lock_filesys);

  /* Return -1 if we can't find the file. */
  return -1;
}


/* Reads size bytes from the file open as fd into buffer. Returns the number of bytes actually read
   (0 at end of file), or -1 if the file could not be read (due to a condition other than end of file).
   Fd 0 reads from the keyboard using input_getc(). */
int read (int fd, void *buffer, unsigned length)
{
  /* list element to iterate the list of file descriptors. */
  struct list_elem *temp;

  lock_acquire(&lock_filesys);

  /* If fd is one, then we must get keyboard input. */
  if (fd == 0)
  {
    lock_release(&lock_filesys);
    return (int) input_getc();
  }

  /* We can't read from standard out, or from a file if we have none open. */
  if (fd == 1 || list_empty(&thread_current()->file_descriptors))
  {
    lock_release(&lock_filesys);
    return 0;
  }

  /* Look to see if the fd is in our list of file descriptors. If found,
     then we read from the file and return the number of bytes written. */
  for (temp = list_front(&thread_current()->file_descriptors); temp != NULL; temp = temp->next)
  {
      struct thread_file *t = list_entry (temp, struct thread_file, file_elem);
      if (t->file_descriptor == fd)
      {
        lock_release(&lock_filesys);
        int bytes = (int) file_read(t->file_addr, buffer, length);
        return bytes;
      }
  }

  lock_release(&lock_filesys);

  /* If we can't read from the file, return -1. */
  return -1;
}

/* Writes LENGTH bytes from BUFFER to the open file FD. Returns the number of bytes actually written,
 which may be less than LENGTH if some bytes could not be written. */
int write (int fd, const void *buffer, unsigned length)
{
  /* list element to iterate the list of file descriptors. */
  struct list_elem *temp;

  lock_acquire(&lock_filesys);

  /* If fd is equal to one, then we write to STDOUT (the console, usually). */
	if(fd == 1)
	{
		putbuf(buffer, length);
    lock_release(&lock_filesys);
    return length;
	}
  /* If the user passes STDIN or no files are present, then return 0. */
  if (fd == 0 || list_empty(&thread_current()->file_descriptors))
  {
    lock_release(&lock_filesys);
    return 0;
  }

  /* Check to see if the given fd is open and owned by the current process. If so, return
     the number of bytes that were written to the file. */
  for (temp = list_front(&thread_current()->file_descriptors); temp != NULL; temp = temp->next)
  {
      struct thread_file *t = list_entry (temp, struct thread_file, file_elem);
      if (t->file_descriptor == fd)
      {
        int bytes_written = (int) file_write(t->file_addr, buffer, length);
        lock_release(&lock_filesys);
        return bytes_written;
      }
  }

  lock_release(&lock_filesys);

  /* If we can't write to the file, return 0. */
  return 0;
}


/* Changes the next byte to be read or written in open file fd to position,
   expressed in bytes from the beginning of the file. (Thus, a position
   of 0 is the file's start.) */
void seek (int fd, unsigned position)
{
  /* list element to iterate the list of file descriptors. */
  struct list_elem *temp;

  lock_acquire(&lock_filesys);

  /* If there are no files to seek through, then we immediately return. */
  if (list_empty(&thread_current()->file_descriptors))
  {
    lock_release(&lock_filesys);
    return;
  }

  /* Look to see if the given fd is in our list of file_descriptors. IF so, then we
     seek through the appropriate file. */
  for (temp = list_front(&thread_current()->file_descriptors); temp != NULL; temp = temp->next)
  {
      struct thread_file *t = list_entry (temp, struct thread_file, file_elem);
      if (t->file_descriptor == fd)
      {
        file_seek(t->file_addr, position);
        lock_release(&lock_filesys);
        return;
      }
  }

  lock_release(&lock_filesys);

  /* If we can't seek, return. */
  return;
}

/* Returns the position of the next byte to be read or written in open file fd,
   expressed in bytes from the beginning of the file. */
unsigned tell (int fd)
{
  /* list element to iterate the list of file descriptors. */
  struct list_elem *temp;

  lock_acquire(&lock_filesys);

  /* If there are no files in our file_descriptors list, return immediately, */
  if (list_empty(&thread_current()->file_descriptors))
  {
    lock_release(&lock_filesys);
    return -1;
  }

  /* Look to see if the given fd is in our list of file_descriptors. If so, then we
     call file_tell() and return the position. */
  for (temp = list_front(&thread_current()->file_descriptors); temp != NULL; temp = temp->next)
  {
      struct thread_file *t = list_entry (temp, struct thread_file, file_elem);
      if (t->file_descriptor == fd)
      {
        unsigned position = (unsigned) file_tell(t->file_addr);
        lock_release(&lock_filesys);
        return position;
      }
  }

  lock_release(&lock_filesys);

  return -1;
}

/* Closes file descriptor fd. Exiting or terminating a process implicitly closes
   all its open file descriptors, as if by calling this function for each one. */
void close (int fd)
{
  /* list element to iterate the list of file descriptors. */
  struct list_elem *temp;

  lock_acquire(&lock_filesys);

  /* If there are no files in our file_descriptors list, return immediately, */
  if (list_empty(&thread_current()->file_descriptors))
  {
    lock_release(&lock_filesys);
    return;
  }

  /* Look to see if the given fd is in our list of file_descriptors. If so, then we
     close the file and remove it from our list of file_descriptors. */
  for (temp = list_front(&thread_current()->file_descriptors); temp != NULL; temp = temp->next)
  {
      struct thread_file *t = list_entry (temp, struct thread_file, file_elem);
      if (t->file_descriptor == fd)
      {
        file_close(t->file_addr);
        list_remove(&t->file_elem);
        lock_release(&lock_filesys);
        return;
      }
  }

  lock_release(&lock_filesys);

  return;
}

/* Check to make sure that the given pointer is in user space,
   and is not null. We must exit the program and free its resources should
   any of these conditions be violated. */
void check_valid_addr (const void *ptr_to_check)
{
  /* Terminate the program with an exit status of -1 if we are passed
     an argument that is not in the user address space or is null. Also make
     sure that pointer doesn't go beyond the bounds of virtual address space.  */
  if(!is_user_vaddr(ptr_to_check) || ptr_to_check == NULL || ptr_to_check < (void *) 0x08048000)
	{
    /* Terminate the program and free its resources */
    exit(-1);
	}
}

/* Ensures that each memory address in a given buffer is in valid user space. */
void check_buffer (void *buff_to_check, unsigned size)
{
  unsigned i;
  char *ptr  = (char * )buff_to_check;
  for (i = 0; i < size; i++)
    {
      check_valid_addr((const void *) ptr);
      ptr++;
    }
}

/* Code inspired by GitHub Repo created by ryantimwilson (full link in Design2.txt).
   Get up to three arguments from a programs stack (they directly follow the system
   call argument). */
void get_stack_arguments (struct intr_frame *f, int *args, int num_of_args)
{
  int i;
  int *ptr;
  for (i = 0; i < num_of_args; i++)
    {
      ptr = (int *) f->esp + i + 1;
      check_valid_addr((const void *) ptr);
      args[i] = *ptr;
    }
}