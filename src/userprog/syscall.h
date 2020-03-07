#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

void close (int fd);
unsigned tell (int fd);
void seek(int fd, unsigned position);

#endif /* userprog/syscall.h */
