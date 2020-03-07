#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

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
  lock_init(&locking_file);
	
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
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
	
	struct element_list * el;
	
	el = list_front(&thread_current()->file_des);
	
	while (el) {
		struct entry_file *f = list_entry (el, struct entry_file, element_file);
		if(fd == f->file_des)
		{
			file_close(f->addr_file);
			list_remove(&f->element_file);
			/*lock_release(&locking_file);*/
			return;
		}
		el = el->next;
	}
	
	/*lock_release(&locking_file);*/
	
	return;

}
