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

static void
syscall_handler (struct intr_frame *f UNUSED)
{
	void * virt_page;
	void * stack_pointer = f->esp;
	validate_address((const void *)stack_pointer);

	switch(*(int *)stack_pointer)
	{
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			exit(arg);
			break;
		case SYS_EXEC:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			virt_page = (void *) pagedir_get_page(thread_current()->pagedir, (const void *) arg);
			if (virt_page == NULL) {
				exit(-1);
			}
			arg = (int) virt_page;
			f->eax = exec((const char *) arg);
			break;
		case SYS_WAIT:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			f->eax = wait((pid_t) arg);
			break;
		case SYS_CREATE:
			int args[2];
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
		case SYS_REMOVE:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			virt_page = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
			if (virt_page == NULL) {
				exit(-1);
			}
			arg = (int) virt_page;
			f->eax = remove((const char *) arg);
			break;
		case SYS_OPEN:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			virt_page = pagedir_get_page(thread_current()->pagedir, (const void *) args[0]);
			if (virt_page == NULL) {
				exit(-1);
			}
			arg = (int) virt_page;
			f->eax = open((const char *) arg);
			break;
		case SYS_FILESIZE:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			f->eax = filesize(arg);
			break;
		case SYS_READ:
			int args[3];
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
		case SYS_WRITE:
			int args[3];
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
		case SYS_SEEK:
			int args[2];
			for (int i = 0; i < 2; i++) {
				stack_pointer = (int *) f->esp + i + 1;
				validate_address((const void *) stack_pointer);
				args[i] = *(int *)stack_pointer;
			}
			seek(args[0], (unsigned) args[1]);
			break;
		case SYS_TELL:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
			f->eax = tell(arg);
			break;
		case SYS_CLOSE:
			stack_pointer = (int *)stack_pointer + 1;
			validate_address((const void *) stack_pointer);
			int arg = *(int *)stack_pointer;
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
	pid_t child_tid = process_execute(file);
	lock_release(&lock_filesys);
	return child_tid;
}

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
	struct entry_file* processFile = malloc(sizeof(*processFile));
	processFile->addr_file = file;
	processFile->des_file = thread_current()->fd_count;
	thread_current()->fd_count++;
	list_push_front(&thread_current()->filedes_list, &processFile->element_file);
	lock_release(&lock_filesys);
	/* Release all locks here */
	return processFile->des_file;
}

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

int read(int fd, void *buffer, unsigned size) 
{
	struct list_elem *e;

	lock_acquire(&lock_filesys);

	if (fd == 0)
	{
		lock_release(&lock_filesys);
		return (int) input_getc();
	}

	for (e = list_begin (&thread_current()->filedes_list); e != list_end (&thread_current()->filedes_list);
		e = list_next (e))
	{
		struct entry_file *f = list_entry (e, struct entry_file, element_file);
		if (f->des_file == fd)
		{
			int bytes_read = (int) file_read(f->addr_file, buffer, size);
			lock_release(&lock_filesys);
			return bytes_read;
		}
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

	for (e = list_begin (&thread_current()->filedes_list); e != list_end (&thread_current()->filedes_list);
	e = list_next (e))
	{
		struct entry_file *f = list_entry (e, struct entry_file, element_file);
		if (f->des_file == fd)
		{
			int bytes_written = (int) file_write(f->addr_file, buffer, size);
			lock_release(&lock_filesys);
			return bytes_written;
		}
	}
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
