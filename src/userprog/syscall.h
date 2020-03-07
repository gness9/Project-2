#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include <debug.h>

void syscall_init (void);

void close (int fd);
unsigned tell (int fd);
void seek(int fd, unsigned position);
int write(int fd, const void *buffer, unsigned size);
int read(int fd, void *buffer, unsigned size);
int filesize(int fd);
int open(const char *file);
bool remove(const char *file);
bool create (const char *file, unsigned initial_size);
void halt (void);

#endif /* userprog/syscall.h */
