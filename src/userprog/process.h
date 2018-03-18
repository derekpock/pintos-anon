#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "list.h"

struct list parentChildList;
struct list parentWaitingOnChildrenList;
struct lock fileIOLock;

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);
void closeFilesFromPid(int pid);

#endif /* userprog/process.h */
