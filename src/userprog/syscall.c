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

/* lock makes sure that file system accesss only has one process at a time */
/*struct lock locking_file;*/

struct entry_file {
	struct file* addr_file;
	int des_file;
	struct list_elem element_file;
};

void
syscall_init (void) 
{
  /*lock_init(&locking_file);*/
	
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if (f->esp == NULL)
  {
    exit(-1);
  }
  
  int * args = f->esp;
  
  switch(*(int*)f->esp) 
  {
    case SYS_HALT:
	  halt();
      break;
    case SYS_EXIT:
	  obtain_arguments(args+1);
	  exit(*(args+1));
      break;
    case SYS_EXEC:
	  obtain_arguments(args+1);
	  obtain_arguments(*(args+1));
	  f->eax = exec(*(args+1));
      break;
    case SYS_WAIT:
	  obtain_arguments(args+1);
	  f->eax = wait(*(args+1));
      break;
    case SYS_CREATE:
	  obtain_arguments(args+5);
	  obtain_arguments(*(args+4));
	  f->eax = create((*(args+4)), (*(args+5)));
      break;
    case SYS_REMOVE:
	  obtain_arguments(args+1);
	  obtain_arguments(*(args+1));
	  f->eax = remove(*(args+1));
      break;
    case SYS_OPEN:
	  obtain_arguments(args+1);
	  obtain_arguments(*(args+1));
      break;
    case SYS_FILESIZE:
	  obtain_arguments(args+1);
	  f->eax = filesize(*(args+1));
      break;
    case SYS_READ:
	  obtain_arguments(args+7);
	  obtain_arguments(*(args+6));
      break;
    case SYS_WRITE:
	  obtain_arguments(args+7);
	  obtain_arguments(*(args+6));
      break;
    case SYS_SEEK:
	  obtain_arguments(args+5);
      break;
    case SYS_TELL:
	  obtain_arguments(args+1);
	  tell(*(args+1));
      break;
    case SYS_CLOSE:
	  obtain_arguments(args+1);
	  close(*(args+1));
      break;	
  }
  thread_exit ();
}

void * obtain_arguments(const void *vaddr) {
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		exit(-1);
		return 0;
	}
	return ptr;
}

/*Terminates Pintos by calling shutdown_power_off()*/
void halt (void)
{
	shutdown_power_off();
}

/*Currently giving large warnings*/
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
	return child_tid;
}


int wait(pid_t pid)
{
	return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
	bool file_create = filesys_create(file, initial_size);
	return file_create;
} 


bool remove (const char *file) 
{
	bool file_remove = filesys_remove(file);
	return file_remove;
}


/* int open(const char *file) 
{
	
	
	
} */

int filesize (int fd) 
{
	struct entry_file *ef = obtain_file(fd);
	if(ef->addr_file != NULL)
	{
		int file_size = file_length(ef->addr_file);
		return file_size;
	}
	return -1;
}



/* int read(int fd, void *buffer, unsigned size) 
{
	
	
	
} */



/* int write(int fd, const void *buffer, unsigned size) 
{
	
	
	
	
} */

/*Changes  the  next  byte  to  be read  or  written  in  open  file fdto position,  
expressed  in  bytes  from  the beginning of the file. (Thus, a positionof 0 is the file's start.) */
void seek (int fd, unsigned position)
{	
	if(list_empty(&thread_current()->filedes_list)) {
		return;
	}
	
	struct entry_file *ef = obtain_file(fd);
	
	if(ef->addr_file != NULL)
	{
		file_seek(ef->addr_file, position);
	}
	
	return;
	
}

/*Returns  the  position  of  the  next  byte  to  be  read  or  written  in  open  file fd,  
expressed  in  bytes  from  the beginning of the file. */
unsigned
tell (int fd)
{
	
	if(list_empty(&thread_current()->filedes_list)) {
		return -1;
	}
	
	struct entry_file *ef = obtain_file(fd);
	
	if(ef->addr_file != NULL)
	{
		unsigned file_pos = (unsigned) file_tell(ef->addr_file);
		return file_pos;
	}
	
	return -1;
	
} 




/*Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors, as if
by calling this function for each one. */
void
close (int fd)
{ 
	/*lock_acquire(&locking_file);*/
	
	if(list_empty(&thread_current()->filedes_list)) {
		/*lock_release(&locking_file);*/
		return;
	}
	
	struct entry_file *ef = obtain_file(fd);
	
	if(ef != NULL)
	{
		file_close(ef->addr_file);
		list_remove(&ef->element_file);
		return;
	}
	
	return;

}

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
