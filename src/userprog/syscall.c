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
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      int exitValue;
      memcpy(&exitValue, f->esp, sizeof(int));  //copy value at esp into exitValue
      f->esp += sizeof(int);  //Move esp "up" to next argument, if exists.

      //Move esp back down to the original value.
      f->esp -= sizeof(int);
      f->esp -= sizeof(void **);


      //Perform operations with data.
      thread_current()->exitCode = exitValue;
      thread_exit();



    case SYS_EXEC:  //const char *file - return pid_t
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      char* file;
      memcpy(&file, f->esp, sizeof(char*));
      int processStatus = process_execute(file);
      f->esp += sizeof(char*);

      //Move esp back down to the original value.
      f->esp -= sizeof(char*);
      f->esp -= sizeof(void**);


      //We know that TID_ERROR has the value of -1 already. No need to re-set it.
//      if(processStatus == TID_ERROR) {
//        processStatus = -1;
//      }
      memcpy(&(f->eax), &processStatus, sizeof(int));
      //TODO don't return until we know that the process started successfully or failed
      //TODO this might be enough to work
      break;



    case SYS_WAIT:  //pid_t pid - return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Move esp back down to the original value.
      f->esp -= sizeof(void **);


      break;



    case SYS_CREATE:    //const char *file, unsigned initial_size - return bool
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      char* newFile;
      memcpy(&newFile, f->esp, sizeof(char*));
      f->esp += sizeof(char*);

      unsigned initialSize;
      memcpy(&initialSize, f->esp, sizeof(unsigned));
      f->esp += sizeof(unsigned);

      //Move esp back down to the original value.
      f->esp -= sizeof(unsigned);
      f->esp -= sizeof(char*);
      f->esp -= sizeof(void **);


      bool isFileCreated = filesys_create(newFile, initialSize);
      memcpy(&(f->eax), &isFileCreated, sizeof(bool));
      break;



    case SYS_REMOVE:    //const char *file - return bool
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //TODO what if removing an open file?
      char* removingFile;
      memcpy(&removingFile, f->esp, sizeof(char*));
      f->esp += sizeof(char*);

      //Move esp back to first position
      f->esp -= sizeof(char *);
      f->esp -= sizeof(void **);


      bool isFileDeleted = filesys_remove(removingFile);
      memcpy(&(f->eax), &isFileDeleted, sizeof(bool));
      break;



    case SYS_OPEN:      //const char *file - return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Move esp back to first position
      f->esp -= sizeof(void **);


      break;



    case SYS_FILESIZE:  //int fd - return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Move esp back to first position
      f->esp -= sizeof(void **);


      break;



    case SYS_READ:      //int fd, void *buffer, unsigned size- return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Move esp back to first position
      f->esp -= sizeof(void **);


      printf("called read\n");
      break;



    case SYS_WRITE:     //int fd, const void *buffer, unsigned size - return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Get argument from the stack (normal getting, this is the first).
      //Getting integer
      int writeFileId;
      memcpy(&writeFileId, f->esp, sizeof(int));
      f->esp += sizeof(int);

      //Get void pointer
      void* writeBuffer;
      memcpy(&writeBuffer, f->esp, sizeof(void *));
      f->esp += sizeof(void *);

      //Getting unsigned value
      unsigned sizeToWrite;
      memcpy(&sizeToWrite, f->esp, sizeof(unsigned)); //copy value at esp into sizeToWrite
      f->esp += sizeof(unsigned); //Move pointer up before this value.

      //Move esp back down to the original value.
      f->esp -= sizeof(int);
      f->esp -= sizeof(void*);
      f->esp -= sizeof(unsigned);
      f->esp -= sizeof(void**);


      //Now perform operations with necessary values.
      if(writeFileId == 1) {
        //We need to write to the console.
        putbuf(writeBuffer, sizeToWrite);
        //Return the entire buffer size
        memcpy(&(f->eax), &sizeToWrite, sizeof(unsigned));
      } else {
        printf("Writing to %d", writeFileId);
        //Find the file..
        //TODO
      }
      break;



    case SYS_SEEK:      //int fd, unsigned position
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Move esp back to first position
      f->esp -= sizeof(void **);


      break;



    case SYS_TELL:      //int fd - return unsigned
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Move esp back to first position
      f->esp -= sizeof(void **);


      break;



    case SYS_CLOSE:     //int fd
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //Move esp back to first position
      f->esp -= sizeof(void **);


      break;



    default:
	    printf("Faulting code: %d\n", call);
      PANIC("Oh noes! This hasn't been implemented yet!");
  }
//  printf ("system call!\n");
//  thread_exit ();
}

