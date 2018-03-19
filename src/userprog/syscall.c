#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include <devices/shutdown.h>
#include <filesys/filesys.h>
#include <devices/input.h>
#include <lib/user/syscall.h>
#include <filesys/file.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include <threads/malloc.h>
#include <threads/vaddr.h>

struct fileItem {
  int fd;
  int pidOwner;
  char* name;
  unsigned position;
  struct file* fsFile;
  struct list_elem elem;
};

static void syscall_handler (struct intr_frame *);
static struct fileItem* createNewFileItem(int fd, int pid, const char* name, struct file* fsFile);
static struct list openFilesList;
static int nextFd;
static bool verifyPointer(void* pointer) {
  return pointer < PHYS_BASE && pointer != NULL;
}

void
syscall_init (void)
{
  list_init(&parentChildList);
  list_init(&parentWaitingOnChildrenList);
  lock_init(&fileIOLock);
  list_init(&openFilesList);
  nextFd = 2;
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

struct fileItem* createNewFileItem(int fd, int pid, const char* name, struct file* fsFile) {
  struct fileItem* item;
  item = malloc(sizeof(struct fileItem));
  if(item == NULL) {
    PANIC("Unable to get memory for newNode!");
  }
  item->fd = fd;
  item->pidOwner = pid;
  item->name = malloc(strlen(name) + 1);
  strlcpy(item->name, name, strlen(name) + 1);
  item->position = 0;
  item->fsFile = fsFile;
  return item;
}


// MUST have file IO lock before calling this
//TODO call this for all force closed processes
static void closeFile(struct fileItem* fileClose) {
  ASSERT(fileIOLock.holder == thread_current());
  list_remove(&fileClose->elem);
  struct list_elem* e;
  bool fileStillOpen = false;
  for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
       e = list_next (e)) {
    struct fileItem *item = list_entry (e, struct fileItem, elem);
    if(item->fsFile == fileClose->fsFile) {
      fileStillOpen = true;
      break;
    }
  }
  if(!fileStillOpen) {
    file_close(fileClose->fsFile);
  }
  free(fileClose->name);
  free(fileClose);
}

void closeFilesFromPid(int pid) {
  lock_acquire(&fileIOLock);
  bool foundFiles = false;
  do {
    struct list_elem* e;
    for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
         e = list_next (e)) {
      struct fileItem *item = list_entry (e, struct fileItem, elem);
      if(item->pidOwner == pid) {
        closeFile(item);
        foundFiles = true;
        break;
      }
    }
  } while(foundFiles);
  lock_release(&fileIOLock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int call;
  struct list_elem *e;
  memcpy(&call, f->esp, sizeof(int));
  switch (call) {
    case SYS_HALT:  //no args
      shutdown_power_off();
      //Done


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
      //Find our pid and set the exit status.
      for (e = list_begin (&threadExit_list); e != list_end (&threadExit_list);
           e = list_next (e)) {
        struct intMap *map = list_entry (e, struct intMap, elem);
        if(map->key == thread_current()->tid) {
          map->value = exitValue;
          break;
        }
      }
      closeFilesFromPid(thread_current()->tid);
      thread_exit();



    case SYS_EXEC:  //const char *file - return pid_t
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      char* file;
      memcpy(&file, f->esp, sizeof(char*));
      f->esp += sizeof(char*);
      if(!verifyPointer(file)) {
        closeFilesFromPid(thread_current()->tid);
        thread_exit();
      }

      //Move esp back down to the original value.
      f->esp -= sizeof(char*);
      f->esp -= sizeof(void**);


      int processStatus = process_execute(file);
      //We know that TID_ERROR has the value of -1 already. No need to re-set it.
      memcpy(&(f->eax), &processStatus, sizeof(int));
      break;



    case SYS_WAIT:  //pid_t pid - return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      pid_t waitOnPid;
      memcpy(&waitOnPid, f->esp, sizeof(pid_t));
      f->esp += sizeof(pid_t);

      //Move esp back down to the original value.
      f->esp -= sizeof(pid_t);
      f->esp -= sizeof(void **);


      int waitResult = process_wait(waitOnPid);
      memcpy(&(f->eax), &waitResult, sizeof(int));
      break;



    case SYS_CREATE:    //const char *file, unsigned initial_size - return bool
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      char* newFile;
      memcpy(&newFile, f->esp, sizeof(char*));
      f->esp += sizeof(char*);
      if(!verifyPointer(newFile)) {
        closeFilesFromPid(thread_current()->tid);
        thread_exit();
      }

      unsigned initialSize;
      memcpy(&initialSize, f->esp, sizeof(unsigned));
      f->esp += sizeof(unsigned);

      //Move esp back down to the original value.
      f->esp -= sizeof(unsigned);
      f->esp -= sizeof(char*);
      f->esp -= sizeof(void **);


      lock_acquire(&fileIOLock);
      bool isFileCreated = filesys_create(newFile, initialSize);
      lock_release(&fileIOLock);

      memcpy(&(f->eax), &isFileCreated, sizeof(bool));
      break;



    case SYS_REMOVE:    //const char *file - return bool
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      //TODO what if removing an open file?
      char* removingFile;
      memcpy(&removingFile, f->esp, sizeof(char*));
      f->esp += sizeof(char*);
      if(!verifyPointer(removingFile)) {
        closeFilesFromPid(thread_current()->tid);
        thread_exit();
      }

      //Move esp back to first position
      f->esp -= sizeof(char *);
      f->esp -= sizeof(void **);


      lock_acquire(&fileIOLock);
      bool isFileDeleted = filesys_remove(removingFile);
      lock_release(&fileIOLock);
      memcpy(&(f->eax), &isFileDeleted, sizeof(bool));
      break;



    case SYS_OPEN:      //const char *file - return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      char* openingFile;
      memcpy(&openingFile, f->esp, sizeof(char*));
      f->esp += sizeof(char*);
      if(!verifyPointer(openingFile)) {
        closeFilesFromPid(thread_current()->tid);
        thread_exit();
      }

      //Move esp back to first position
      f->esp -= sizeof(char*);
      f->esp -= sizeof(void **);

      //Check if the file is already open.
      int openFileRetVal = -1;
      bool isFileOpen = false;
      lock_acquire(&fileIOLock);
      for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
           e = list_next (e)) {
        struct fileItem *item = list_entry (e, struct fileItem, elem);
        if(strcmp(openingFile, item->name) == 0) {
          isFileOpen = true;
          struct fileItem* newFileItem = createNewFileItem(nextFd++, thread_current()->tid, openingFile, item->fsFile);
          list_push_back(&openFilesList, &newFileItem->elem);
          openFileRetVal = newFileItem->fd;
          break;
        }
      }
      if(!isFileOpen) {
        struct file* fsFile = filesys_open(openingFile);
        if(fsFile != NULL) {
          struct fileItem* newFileItem = createNewFileItem(nextFd++, thread_current()->tid, openingFile, fsFile);
          list_push_back(&openFilesList, &newFileItem->elem);
          openFileRetVal = newFileItem->fd;
        }
      }
      lock_release(&fileIOLock);
      memcpy(&(f->eax), &openFileRetVal, sizeof(int));

      break;



    case SYS_FILESIZE:  //int fd - return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      int fileSizeCheckFd;
      memcpy(&fileSizeCheckFd, f->esp, sizeof(int));
      f->esp += sizeof(int);

      //Move esp back to first position
      f->esp -= sizeof(int);
      f->esp -= sizeof(void **);


      off_t fileSizeCheckRetVal = -1;
      lock_acquire(&fileIOLock);
      for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
           e = list_next (e)) {
        struct fileItem *item = list_entry (e, struct fileItem, elem);
        if(item->fd == fileSizeCheckFd) {
          if(item->pidOwner != thread_current()->tid) {
            //doesn't own the open file!
          } else {
            fileSizeCheckRetVal = file_length(item->fsFile);
          }
          break;
        }
      }
      lock_release(&fileIOLock);
      memcpy(&(f->eax), &fileSizeCheckRetVal, sizeof(off_t));
      break;



    case SYS_READ:      //int fd, void *buffer, unsigned size- return int
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      int readFileId;
      memcpy(&readFileId, f->esp, sizeof(int));
      f->esp += sizeof(int);

      char *bufferRead;
      memcpy(&bufferRead, f->esp, sizeof(void*));
      f->esp += sizeof(void*);
      if(!verifyPointer(bufferRead)) {
        closeFilesFromPid(thread_current()->tid);
        thread_exit();
      }

      unsigned requestedReadSize;
      memcpy(&requestedReadSize, f->esp, sizeof(unsigned));
      f->esp += sizeof(unsigned);

      //Move esp back to first position
      f->esp -= sizeof(unsigned);
      f->esp -= sizeof(void*);
      f->esp -= sizeof(int);
      f->esp -= sizeof(void **);

      off_t readBytes = -1;
      if(readFileId == 0) {
        //Read from stdin
        for(int bufferPosition = 0;
            requestedReadSize - (bufferPosition * sizeof(uint8_t)) > 0;
            bufferPosition++) {
          uint8_t byte = input_getc();
          *(bufferRead + bufferPosition) = byte;
        }
        readBytes = requestedReadSize;
      } else {
        lock_acquire(&fileIOLock);
        for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
             e = list_next (e)) {
          struct fileItem *item = list_entry (e, struct fileItem, elem);
          if(item->fd == readFileId) {
            if(item->pidOwner != thread_current()->tid) {
              //Doesn't own the file
            } else {
              readBytes = file_read_at(item->fsFile, bufferRead, requestedReadSize, item->position);
              item->position += readBytes;
            }
            break;
          }
        }
        lock_release(&fileIOLock);
      }
      memcpy(&(f->eax), &readBytes, sizeof(off_t));
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
      if(!verifyPointer(writeBuffer)) {
        closeFilesFromPid(thread_current()->tid);
        thread_exit();
      }

      //Getting unsigned value
      unsigned sizeToWrite;
      memcpy(&sizeToWrite, f->esp, sizeof(unsigned)); //copy value at esp into sizeToWrite
      f->esp += sizeof(unsigned); //Move pointer up before this value.

      //Move esp back down to the original value.
      f->esp -= sizeof(int);
      f->esp -= sizeof(void*);
      f->esp -= sizeof(unsigned);
      f->esp -= sizeof(void**);


      off_t bytesWritten = -1;
      //Now perform operations with necessary values.
      if(writeFileId == 1) {
        //We need to write to the console.
        putbuf(writeBuffer, sizeToWrite);
        //Return the entire buffer size
        memcpy(&(f->eax), &sizeToWrite, sizeof(unsigned));
      } else {
        lock_acquire(&fileIOLock);
        for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
             e = list_next (e)) {
          struct fileItem *item = list_entry (e, struct fileItem, elem);
          if(item->fd == writeFileId) {
            if(item->pidOwner != thread_current()->tid) {
              //Doesn't own the file
            } else {
              bytesWritten = file_write_at(item->fsFile, writeBuffer, sizeToWrite, item->position);
              item->position += bytesWritten;
            }
            break;
          }
        }
        lock_release(&fileIOLock);
      }
      memcpy(&(f->eax), &bytesWritten, sizeof(off_t));
      break;



    case SYS_SEEK:      //int fd, unsigned position
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      int fdSeek;
      memcpy(&fdSeek, f->esp, sizeof(int));
      f->esp += sizeof(int);

      unsigned fdPosition;
      memcpy(&fdPosition, f->esp, sizeof(unsigned));
      f->esp += sizeof(unsigned);

      //Move esp back to first position
      f->esp -= sizeof(unsigned);
      f->esp -= sizeof(int);
      f->esp -= sizeof(void **);


      lock_acquire(&fileIOLock);
      for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
           e = list_next (e)) {
        struct fileItem *item = list_entry (e, struct fileItem, elem);
        if(item->fd == fdSeek) {
          if(item->pidOwner != thread_current()->tid) {
            //Doesn't own the file
          } else {
            item->position = fdPosition;
          }
          break;
        }
      }
      lock_release(&fileIOLock);
      break;



    case SYS_TELL:      //int fd - return unsigned
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      int fdTell;
      memcpy(&fdTell, f->esp, sizeof(int));
      f->esp += sizeof(int);

      //Move esp back to first position
      f->esp -= sizeof(int);
      f->esp -= sizeof(void **);


      unsigned tellValue = 0;
      lock_acquire(&fileIOLock);
      for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
           e = list_next (e)) {
        struct fileItem *item = list_entry (e, struct fileItem, elem);
        if(item->fd == fdTell) {
          if(item->pidOwner != thread_current()->tid) {
            //Doesn't own the file - TODO
          } else {
            tellValue = item->position;
          }
          break;
        }
      }
      lock_release(&fileIOLock);
      memcpy(&(f->eax), &tellValue, sizeof(unsigned));
      break;



    case SYS_CLOSE:     //int fd
      //Move pointer back before the return value, to the first argument on the stack.
      f->esp += sizeof(void **);

      int fdClose;
      memcpy(&fdClose, f->esp, sizeof(int));
      f->esp += sizeof(int);

      //Move esp back to first position
      f->esp -= sizeof(int);
      f->esp -= sizeof(void **);


      lock_acquire(&fileIOLock);
      for (e = list_begin (&openFilesList); e != list_end (&openFilesList);
           e = list_next (e)) {
        struct fileItem *item = list_entry (e, struct fileItem, elem);
        if(item->fd == fdClose) {
          if(item->pidOwner != thread_current()->tid) {
            //Doesn't own the file - TODO
          } else {
            closeFile(item);
          }
          break;
        }
      }
      lock_release(&fileIOLock);
      break;



    default:
	    printf("Faulting code: %d\n", call);
      PANIC("Oh noes! This hasn't been implemented yet!");
  }
//  printf ("system call!\n");
//  thread_exit ();
}

