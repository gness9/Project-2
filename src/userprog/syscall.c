#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
struct entry_file * obtain_file(int fd);
void * obtain_arguments(const void *vaddr);

void get_stack_arguments (struct intr_frame *f, int * args, int num_of_args);

/* lock makes sure that file system accesss only has one process at a time 
STILL NEEDS TO BE IMPLEMENTED*/
/*struct lock locking_file;*/

struct entry_file {
	struct file* addr_file;
	int des_file;
	struct list_elem element_file;
};

/* Lock is in charge of ensuring that only one process can access the file system at one time. */
struct lock lock_filesys;

void
syscall_init (void) 
{
  lock_init(&lock_filesys);
	
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


/* Handles a system call initiated by a user program. */
static void
syscall_handler (struct intr_frame *f UNUSED)
{
    /* First ensure that the system call argument is a valid address. If not, exit immediately. */
    check_valid_addr((const void *) f->esp);

    /* Holds the stack arguments that directly follow the system call. */
    int args[3];

    /* Stores the physical page pointer. */
    void * phys_page_ptr;

		/* Get the value of the system call (based on enum) and call corresponding syscall function. */
		switch(*(int *) f->esp)
		{
			case SYS_HALT:
        /* Call the halt() function, which requires no arguments */
				halt();
				break;

			case SYS_EXIT:
        /* Exit has exactly one stack argument, representing the exit status. */
        get_stack_arguments(f, &args[0], 1);

				/* We pass exit the status code of the process. */
				exit(args[0]);
				break;

			case SYS_EXEC:
				/* The first argument of exec is the entire command line text for executing the program */
				get_stack_arguments(f, &args[0], 1);

        /* Ensures that converted address is valid. */
        phys_page_ptr = (void *) pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
        if (phys_page_ptr == NULL)
        {
          exit(-1);
        }
        args[0] = (int) phys_page_ptr;

        /* Return the result of the exec() function in the eax register. */
				f->eax = exec((const char *) args[0]);
				break;

			case SYS_WAIT:
        /* The first argument is the PID of the child process
           that the current process must wait on. */
				get_stack_arguments(f, &args[0], 1);

        /* Return the result of the wait() function in the eax register. */
				f->eax = wait((pid_t) args[0]);
				break;

			case SYS_CREATE:
        /* The first argument is the name of the file being created,
           and the second argument is the size of the file. */
				get_stack_arguments(f, &args[0], 2);
        check_buffer((void *)args[0], args[1]);

        /* Ensures that converted address is valid. */
        phys_page_ptr = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
        if (phys_page_ptr == NULL)
        {
          exit(-1);
        }
        args[0] = (int) phys_page_ptr;

        /* Return the result of the create() function in the eax register. */
        f->eax = create((const char *) args[0], (unsigned) args[1]);
				break;

			case SYS_REMOVE:
        /* The first argument of remove is the file name to be removed. */
        get_stack_arguments(f, &args[0], 1);

        /* Ensures that converted address is valid. */
        phys_page_ptr = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
        if (phys_page_ptr == NULL)
        {
          exit(-1);
        }
        args[0] = (int) phys_page_ptr;

        /* Return the result of the remove() function in the eax register. */
        f->eax = remove((const char *) args[0]);
				break;

			case SYS_OPEN:
        /* The first argument is the name of the file to be opened. */
        get_stack_arguments(f, &args[0], 1);

        /* Ensures that converted address is valid. */
        phys_page_ptr = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
        if (phys_page_ptr == NULL)
        {
          exit(-1);
        }
        args[0] = (int) phys_page_ptr;

        /* Return the result of the remove() function in the eax register. */
        f->eax = open((const char *) args[0]);

				break;

			case SYS_FILESIZE:
        /* filesize has exactly one stack argument, representing the fd of the file. */
        get_stack_arguments(f, &args[0], 1);

        /* We return file size of the fd to the process. */
        f->eax = filesize(args[0]);
				break;

			case SYS_READ:
        /* Get three arguments off of the stack. The first represents the fd, the second
           represents the buffer, and the third represents the buffer length. */
        get_stack_arguments(f, &args[0], 3);

        /* Make sure the whole buffer is valid. */
        check_buffer((void *)args[1], args[2]);

        /* Ensures that converted address is valid. */
        phys_page_ptr = pagedir_get_page(thread_current()->pagedir, (const void *) args[1]);
        if (phys_page_ptr == NULL)
        {
          exit(-1);
        }
        args[1] = (int) phys_page_ptr;

        /* Return the result of the read() function in the eax register. */
        f->eax = read(args[0], (void *) args[1], (unsigned) args[2]);
				break;

			case SYS_WRITE:
        /* Get three arguments off of the stack. The first represents the fd, the second
           represents the buffer, and the third represents the buffer length. */
        get_stack_arguments(f, &args[0], 3);

        /* Make sure the whole buffer is valid. */
        check_buffer((void *)args[1], args[2]);

        /* Ensures that converted address is valid. */
        phys_page_ptr = pagedir_get_page(thread_current()->pagedir, (const void *) args[1]);
        if (phys_page_ptr == NULL)
        {
          exit(-1);
        }
        args[1] = (int) phys_page_ptr;

        /* Return the result of the write() function in the eax register. */
        f->eax = write(args[0], (const void *) args[1], (unsigned) args[2]);
        break;

			case SYS_SEEK:
        /* Get two arguments off of the stack. The first represents the fd, the second
           represents the position. */
        get_stack_arguments(f, &args[0], 2);

        /* Return the result of the seek() function in the eax register. */
        seek(args[0], (unsigned) args[1]);
        break;

			case SYS_TELL:
        /* tell has exactly one stack argument, representing the fd of the file. */
        get_stack_arguments(f, &args[0], 1);

        /* We return the position of the next byte to read or write in the fd. */
        f->eax = tell(args[0]);
        break;

			case SYS_CLOSE:
        /* close has exactly one stack argument, representing the fd of the file. */
        get_stack_arguments(f, &args[0], 1);

        /* We close the file referenced by the fd. */
        close(args[0]);
				break;

			default:
        /* If an invalid system call was sent, terminate the program. */
				exit(-1);
				break;
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

/*Implement  the  system  call  handler  in userprog/syscall.c.  
The  skeleton  implementation  we  provide  "handles" system  calls  by  terminating  the  process.  
It  will  need  to  retrieve  the  system  call  number,  
then  any  system  call arguments, and carry out appropriate actions.*/
/* static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	
  check_valid_addr((const void *) f->esp);

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


/*Terminates Pintos by calling shutdown_power_off()*/
void halt (void)
{
	shutdown_power_off();
}

/*Terminates the current user program, returning statusto the kernel. 
If the process's parent waits for it (see below), this is the status that will be returned.*/
void exit(int status) 
{
	thread_current()->status_exit = status;
	printf("%s: exit(%d)\n", thread_current()->name, status);
	thread_exit ();
}

/*Runs the  executable  whose name  is given in cmd_line, passing any given arguments, 
and returns the new process's  program  id (pid). */
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

/*Creates a  new file called fileinitially initial_sizebytes in size. 
Returns true  if successful,  false otherwise. Creating  a  new  file  does  not  open  it:  
opening  the  new  file  is  a  separate  operation  which  would  require  a opensystem call. */
bool create (const char *file, unsigned initial_size)
{
	lock_acquire(&lock_filesys);
	bool file_create = filesys_create(file, initial_size);
	lock_release(&lock_filesys);
	return file_create;
} 

/*Deletes the file called file. Returns true if successful, false otherwise. 
A file may be removed regardless of whether it is open or closed, 
and removing an open file does not close it. See Removing an Open File, for details. */
bool remove (const char *file) 
{
	lock_acquire(&lock_filesys);
	bool file_remove = filesys_remove(file);
	lock_release(&lock_filesys);
	return file_remove;
}

/*Opens  the  file  called file.  Returns  a  nonnegative  integer  handle 
called  a  "file  descriptor"  (fd),  or -1  if  the file could not be opened. */
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
	struct entry_file* process_file = malloc(sizeof(*process_file));
	process_file->addr_file = file;
	process_file->des_file = thread_current()->cur_fd;
	thread_current()->cur_fd++;
	list_push_front(&thread_current()->filedes_list, &process_file->element_file);
	lock_release(&lock_filesys);
	/* Release all locks here */
	return process_file->des_file;
}

/*Returns the size, in bytes, of the file open as fd. */
int filesize (int fd) 
{
	lock_acquire(&lock_filesys);
	struct entry_file *ef = obtain_file(fd);
	if(ef->addr_file != NULL)
	{
		int file_size = file_length(ef->addr_file);
		lock_release(&lock_filesys);
		return file_size;
	}
	lock_release(&lock_filesys);
	return -1;
}

/*Reads sizebytes from the file open as fdinto buffer. Returns the number of 
bytes actually read (0 at end of file),  or -1  if  the  file  could  not  be  read */
int read(int fd, void *buffer, unsigned size) 
{
	struct list_elem *e;

	lock_acquire(&lock_filesys);

	if (fd == 0)
	{
		lock_release(&lock_filesys);
		return (int) input_getc();
	}
	
	struct entry_file *ef = obtain_file(fd);
	
	if(ef->addr_file != NULL)
	{
		int bytes_read = (int) file_read(ef->addr_file, buffer, size);
		lock_release(&lock_filesys);
		return bytes_read;
	}

	lock_release(&lock_filesys);
	return -1;
}

/*Writes sizebytes from bufferto the open file fd. Returns the number of bytes actually written, 
which may be less than sizeif some bytes could not be written. */
int write(int fd, const void *buffer, unsigned size) 
{
	struct list_elem *e;
	
	lock_acquire(&lock_filesys);

	if (fd == 1)
	{
		putbuf(buffer, size);
		lock_release(&lock_filesys);
		return size;
	}

	struct entry_file *ef = obtain_file(fd);
	
	if(ef->addr_file != NULL)
	{
		int bytes_written = (int) file_write(ef->addr_file, buffer, size);
		lock_release(&lock_filesys);
		return bytes_written;
	}
	lock_release(&lock_filesys);
	return 0;
}

/*Changes  the  next  byte  to  be read  or  written  in  open  file fdto position,  
expressed  in  bytes  from  the beginning of the file. (Thus, a positionof 0 is the file's start.) */
void seek (int fd, unsigned position)
{	
	
	lock_acquire(&lock_filesys);
	
	if(list_empty(&thread_current()->filedes_list)) {
		lock_release(&lock_filesys);
		return;
	}
	
	struct entry_file *ef = obtain_file(fd);
	
	if(ef->addr_file != NULL)
	{
		file_seek(ef->addr_file, position);
	}
	
	lock_release(&lock_filesys);
	return;
	
}

/*Returns  the  position  of  the  next  byte  to  be  read  or  written  in  open  file fd,  
expressed  in  bytes  from  the beginning of the file. */
unsigned
tell (int fd)
{
	
	lock_acquire(&lock_filesys);
	
	if(list_empty(&thread_current()->filedes_list)) {
		lock_release(&lock_filesys);
		return -1;
	}
	
	struct entry_file *ef = obtain_file(fd);
	
	if(ef->addr_file != NULL)
	{
		unsigned file_pos = (unsigned) file_tell(ef->addr_file);
		lock_release(&lock_filesys);
		return file_pos;
	}
	
	lock_release(&lock_filesys);
	return -1;
	
}

/*Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors, as if
by calling this function for each one. */
void
close (int fd)
{ 

	lock_acquire(&lock_filesys);
	/*lock_acquire(&locking_file);*/
	
	if(list_empty(&thread_current()->filedes_list)) {
		/*lock_release(&locking_file);*/
		lock_release(&lock_filesys);
		return;
	}
	
	struct entry_file *ef = obtain_file(fd);
	
	if(ef != NULL)
	{
		file_close(ef->addr_file);
		list_remove(&ef->element_file);
		lock_release(&lock_filesys);
		return;
	}
	
	lock_release(&lock_filesys);
	return;

}

/*Based on the file descriptor, gets a file from the list of files*/
struct entry_file * obtain_file(int fd) {
	
	struct list_elem * el;
	
	el = list_front(&thread_current()->filedes_list);
	
	while (el != NULL) {
		struct entry_file *f = list_entry (el, struct entry_file, element_file);
		if(fd == f->des_file)
		{
			return f;
		}
		el = el->next;
	}
	return NULL;
}
