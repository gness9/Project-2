#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

void close (int fd);
unsigned tell (int fd);
void seek(int fd, unsigned position);
int write(int fd, const void *buffer, unsigned size);
int read(int fd, void *buffer, unsigned size);
int filesize(int fd);

#endif /* userprog/syscall.h */
