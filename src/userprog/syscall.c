#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include <devices/shutdown.h>
#include <filesys/filesys.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int call;
  memcpy(&call, f->esp, sizeof(int));
  switch (call) {
    case SYS_HALT:  //no args
      shutdown_power_off();
    case SYS_EXIT:  //int intArg1
      f->esp += sizeof(int);
      int exitValue;
      memcpy(&exitValue, f->esp, sizeof(int));
      thread_current()->exitCode = exitValue;
      thread_exit();
    case SYS_EXEC:  //const char *file - return pid_t
      f->esp += sizeof(char**);
      char* file;
      memcpy(&file, f->esp, sizeof(char**));
      int processStatus = process_execute(file);
      //We know that TID_ERROR has the value of -1 already. No need to re-set it.
//      if(processStatus == TID_ERROR) {
//        processStatus = -1;
//      }
      memcpy(&(f->eax), &processStatus, sizeof(int));
      //TODO don't return until we know that the process started successfully or failed
      //TODO this might be enough to work
      break;
    case SYS_WAIT:  //pid_t pid - return int
      break;
    case SYS_CREATE:    //unsigned initial_size, const char *file - return bool
      f->esp += sizeof(unsigned);
      unsigned initialSize;
      memcpy(&initialSize, f->esp, sizeof(unsigned));

      f->esp += sizeof(char**);
      char* newFile;
      memcpy(&newFile, f->esp, sizeof(char**));

      bool isFileCreated = filesys_create(newFile, initialSize);
      memcpy(&(f->eax), &isFileCreated, sizeof(bool));
      break;
    case SYS_REMOVE:    //const char *file - return bool
      //TODO what if removing an open file?
      f->esp += sizeof(char**);
      char* removingFile;
      memcpy(&removingFile, f->esp, sizeof(char**));

      bool isFileDeleted = filesys_remove(removingFile);
      memcpy(&(f->eax), &isFileDeleted, sizeof(bool));
      break;
    case SYS_OPEN:      //const char *file - return int
      break;
    case SYS_FILESIZE:  //int fd - return int

      break;
    case SYS_READ:      //unsigned size, void *buffer, int fd - return int
      printf("called read\n");
      break;
    case SYS_WRITE:     //unsigned size, const void *buffer, int fd - return int
      printf("called write\n");
      break;
    case SYS_SEEK:      //unsigned position, int fd
      break;
    case SYS_TELL:      //int fd - return unsigned
      break;
    case SYS_CLOSE:     //int fd
      break;
    default:
	    printf("Faulting code: %d\n", call);
      PANIC("Oh noes! This hasn't been implemented yet!");
  }

  printf ("system call!\n");
  thread_exit ();
}

