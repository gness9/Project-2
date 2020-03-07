#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
#include <debug.h>

typedef int pid_t;

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
int wait(pid_t pid);
pid_t exec (const char * file);
void exit(int status);
void halt (void);

#endif /* userprog/syscall.h */
