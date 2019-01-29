#pragma warning (disable : 4996)
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "sys/shm.h"
#include "sys/sem.h"
#include "sys/ipc.h"

static int ftokTableUsed;
static int ftokTableSize;

static char** ftokTable;

#define SUPPORT_ERRNO 1

SECURITY_ATTRIBUTES inheritable = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

#ifdef SUPPORT_ERRNO
int translateWin32ErrorToErrno() { 
    switch (GetLastError()) { 
      case ERROR_SUCCESS: /* The operation completed successfully.  */
	return 0;
      case ERROR_INVALID_FUNCTION: /* Incorrect function.  */
	return EINVAL;
      case ERROR_FILE_NOT_FOUND: /* The system cannot find the file specified.  */
	return EMFILE;       
      case ERROR_PATH_NOT_FOUND: /* The system cannot find the path specified.  */
	return ENOENT;       
      case ERROR_TOO_MANY_OPEN_FILES: /* The system cannot open the file.  */
	return ENOMEM;
      case ERROR_ACCESS_DENIED: /* Access is denied.  */
	return EACCES;
      case ERROR_INVALID_HANDLE: /* The handle is invalid.  */
	return EBADF;
      case ERROR_ARENA_TRASHED: /* The storage control blocks were destroyed.  */
	return ENFILE;
      case ERROR_NOT_ENOUGH_MEMORY: /* Not enough storage is available to process this command.  */
	return ENOMEM;
      case ERROR_INVALID_BLOCK: /* The storage control block address is invalid.  */
	return EBADF;
      case ERROR_BAD_ENVIRONMENT: /*  The environment is incorrect.  */
	return EBADF;
      case ERROR_BAD_FORMAT: /*  An attempt was made to load a program with an incorrect format.  */
	return EBADF;
      case ERROR_INVALID_ACCESS: /*  The access code is invalid.  */
	return EACCES;
      case ERROR_INVALID_DATA: /*  The data is invalid.  */
	return EINVAL;
      case ERROR_OUTOFMEMORY: /*  Not enough storage is available to complete this operation.  */
	return ENOMEM;
      case ERROR_INVALID_DRIVE: /*  The system cannot find the drive specified.  */
	return ENODEV;
      case ERROR_CURRENT_DIRECTORY: /*  The directory cannot be removed.  */
	return EACCES;
      case ERROR_NOT_SAME_DEVICE: /*  The system cannot move the file to a different disk drive.  */
	return EXDEV;
      case ERROR_NO_MORE_FILES: /*  There are no more files.  */
	return ENOENT;       	
      case ERROR_WRITE_PROTECT: /*  The media is write protected.  */
	return EROFS;       
      case ERROR_BAD_UNIT: /*  The system cannot find the device specified.  */
	return ENXIO;
      case ERROR_NOT_READY: /*  The device is not ready.  */
	return EBUSY;
      case ERROR_BAD_COMMAND: /*  The device does not recognize the command.  */
	return EINVAL;
      case ERROR_CRC: /*  Data error (cyclic redundancy check).  */
      case ERROR_BAD_EXE_FORMAT:
	return ENOEXEC;
      case ERROR_BAD_LENGTH: /*  The program issued a command but the command length is incorrect.  */
	return ENOEXEC;
      case ERROR_SEEK: /*  The drive cannot locate a specific area or track on the disk.  */
	return ESPIPE;
      case ERROR_NOT_DOS_DISK: /*  The specified disk or diskette cannot be accessed.  */
	return ENODEV;
      case ERROR_SECTOR_NOT_FOUND: /*  The drive cannot find the sector requested.  */
	return EIO;
      case ERROR_OUT_OF_PAPER: /*  The printer is out of paper.  */
	return EIO;
      case ERROR_WRITE_FAULT: /*  The system cannot write to the specified device.  */
	return EIO;	
      case ERROR_READ_FAULT: /*  The system cannot read from the specified device.  */
	return EIO;
      case ERROR_GEN_FAILURE: /*  A device attached to the system is not functioning.  */
	return ENXIO;	
      case ERROR_SHARING_VIOLATION: /*  The process cannot access the file because it is being used by another process.  */
	return EACCES;
      case ERROR_LOCK_VIOLATION: /*  The process cannot access the file because another process has locked a portion of the file.  */
	return EACCES;
      case ERROR_WRONG_DISK: /*  The wrong diskette is in the drive. Insert %2 (Volume Serial Number: %3) into drive %1.  */
	return EBADF;
      case ERROR_SHARING_BUFFER_EXCEEDED: /*  Too many files opened for sharing.  */
	return EBADF;
      case ERROR_HANDLE_EOF: /*  Reached the end of the file.  */
	return EIO;
      case ERROR_HANDLE_DISK_FULL: /*  The disk is full.  */
      case ERROR_DISK_FULL:
	return ENOSPC;
      case ERROR_BUFFER_OVERFLOW:
	return ENOMEM;
      case ERROR_ALREADY_EXISTS:
	return EEXIST; 
      case ERROR_TOO_MANY_SEMAPHORES:
	return ENOSPC;
      case ERROR_BUSY:
	return EBUSY;
      case ERROR_INVALID_FLAG_NUMBER:
	return EINVAL;
      case ERROR_SEM_NOT_FOUND:
	return ENOENT;
      case ERROR_LOCKED:
	return EACCES;
      case ERROR_DIRECTORY:
	return ENOTDIR;
      case ERROR_OPERATION_ABORTED:
	return EINTR;
      case ERROR_IO_INCOMPLETE:
      case ERROR_IO_PENDING:
	return EAGAIN;
      case ERROR_RETRY:
	return EAGAIN;
      case ERROR_FILE_INVALID:
	return EBADF;
      default:
	return EINVAL;
    }
}
   
#define ERRNO() errno = translateWin32ErrorToErrno()
#define SET_ERRNO(x) errno = x
#else
#define ERRNO() 
#define SET_ERRNO(x)
#endif

typedef struct ShmemDescriptor { 
    HANDLE fh;
    HANDLE mh;
    char*  fileName;
} ShmemDescriptor;


void *shmat(int shmid, const void *shmaddr, int shmflg)
{
    ShmemDescriptor* desc = (ShmemDescriptor*)shmid;
    void* addr = MapViewOfFileEx(desc->mh, 
				 (shmflg & SHM_RDONLY) != 0 ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, 
				 0, 0, 0, (void*)shmaddr);
    if (addr == NULL) { 
	ERRNO();
	return (void*)-1;
    } else {
	return addr;
    }
}

int shmdt(const void *shmaddr)
{
    if (!UnmapViewOfFile((void*)shmaddr)) { 
	ERRNO();
	return -1;
    }
    return 0;
}

int shmget(key_t key, size_t size, int shmflg)
{
    char   buf[64];
    char*  name, *fileName = NULL;
    HANDLE f = INVALID_HANDLE_VALUE, h;
    ShmemDescriptor* desc;

    if (key == IPC_PRIVATE) { 
	name = NULL;
    } else if (key <= ftokTableUsed) { 
	name = ftokTable[key-1];
    } else { 
	sprintf(buf, "key.%ld", key);
	name = buf;
    }
    if (name != NULL) { 
	char tempFilePath[256];
	int  len;
	tempFilePath[0] = '\0';
	GetTempPath(sizeof(tempFilePath), tempFilePath);
	len = strlen(tempFilePath);
	if (len > 0 && tempFilePath[len-1] != '\\') { 
	    tempFilePath[len++] = '\\';
	}
	sprintf(tempFilePath + len, "%s.shm", name);
	f = CreateFile(tempFilePath, GENERIC_READ|GENERIC_WRITE, 
		       FILE_SHARE_READ|FILE_SHARE_WRITE, &inheritable, 
		       (shmflg & IPC_CREAT) 
		         ? (shmflg & IPC_EXCL) ? CREATE_NEW : OPEN_ALWAYS 
		         : OPEN_EXISTING,
		       FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (f == INVALID_HANDLE_VALUE) { 
	    ERRNO();
	    return -1;
	}
	fileName = strdup(tempFilePath);
    }
    h = CreateFileMapping(f, &inheritable, 
			  (shmflg & SHM_W) == 0 ? PAGE_READONLY : PAGE_READWRITE,
			  0, size, name);
    if (h == NULL) { 
	ERRNO();
	if (f != INVALID_HANDLE_VALUE) { 
	    CloseHandle(f);
	}
	return -1;
    }
    desc = (ShmemDescriptor*)malloc(sizeof(ShmemDescriptor));
    desc->fh = f;
    desc->mh = h;
    desc->fileName = fileName;
    return (int)desc;
}

int shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
    ShmemDescriptor* desc = (ShmemDescriptor*)shmid;
    if (cmd == IPC_RMID) { 
	if (!CloseHandle(desc->mh)) { 
	    ERRNO();
	    return -1;
	}
	if (desc->fh != INVALID_HANDLE_VALUE){ 
	    if (!CloseHandle(desc->fh)) { 
		ERRNO();
		return -1;
	    }
	    if (desc->fileName != NULL) { 
		DeleteFile(desc->fileName);
		free(desc->fileName);
	    }
	}
	free(desc);
	return 0;     
    }
    SET_ERRNO(ENOSYS); 
    return -1;
}

key_t ftok(char *pathname, char proj)
{
    if (ftokTableUsed == ftokTableSize) { 
	char** newTable = (char**)malloc(ftokTableSize*2*sizeof(char*));
	memcpy(newTable, ftokTable, ftokTableUsed*sizeof(char*));
	if (ftokTable != NULL) { 
	    free(ftokTable);
	}
	ftokTableSize *= 2;
	ftokTable = newTable;
    }
    ftokTable[ftokTableUsed] = (char*)malloc(strlen(pathname) + 4);
    sprintf(ftokTable[ftokTableUsed], "%s.%02x", pathname, proj & 0xFF);
    return ++ftokTableUsed;
}

typedef struct SemaphoreData { 
    volatile int waitCount;
    volatile int value[1];
} SemaphoreData;

typedef struct SemaphoreDescriptor { 
    unsigned       nSems;
    SemaphoreData* data;
    HANDLE         shmem;
    HANDLE         mutex;
    HANDLE         event;
    HANDLE         sem;
    HANDLE         fh;
    char*          fileName;
} SemaphoreDescriptor;

int semget(key_t key, int nsems, int semflg)
{
    char   buf[1024];
    char*  name;
    char*  fileName = NULL;
    int    exists;
    HANDLE shmem, mutex, event, sem, f = INVALID_HANDLE_VALUE;
    SemaphoreData*       data;
    SemaphoreDescriptor* desc;

    if (key == IPC_PRIVATE) { 
	name = NULL;
    } else if (key <= ftokTableUsed) { 
	sprintf(buf, "%s.shm", ftokTable[key-1]);
	name = buf;
    } else { 
	sprintf(buf, "key.%ld.shm", key);
	name = buf;
    }
    if (name != NULL) { 
	char tempFilePath[256];
	int len;
	tempFilePath[0] = '\0';
	GetTempPath(sizeof(tempFilePath), tempFilePath);
	len = strlen(tempFilePath);
	if (len > 0 && tempFilePath[len-1] != '\\') { 
	    tempFilePath[len++] = '\\';
	}
	sprintf(tempFilePath + len, "%s.sem", name);
	f = CreateFile(tempFilePath, GENERIC_READ|GENERIC_WRITE, 
		       FILE_SHARE_READ|FILE_SHARE_WRITE, &inheritable, 
		       (semflg & IPC_CREAT) 
		         ? (semflg & IPC_EXCL) ? CREATE_NEW : OPEN_ALWAYS 
		         : OPEN_EXISTING,
		       FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (f == INVALID_HANDLE_VALUE) { 
	    ERRNO();
	    return -1;
	}
	fileName = strdup(tempFilePath);
    }
    shmem = CreateFileMapping(f, &inheritable, PAGE_READWRITE,
			      0, sizeof(SemaphoreData) + (nsems-1)*sizeof(int), name);
    if (shmem == NULL) { 
	ERRNO();
	if (f != INVALID_HANDLE_VALUE) {
	    CloseHandle(f);
	}
	return -1;
    }
    exists = GetLastError() == ERROR_ALREADY_EXISTS;
    data = (SemaphoreData*)MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (data == NULL) { 
	ERRNO();
	CloseHandle(shmem);
	if (f != INVALID_HANDLE_VALUE) {
	    CloseHandle(f);
	}
	return -1;
    }
    if (!exists) { 
	memset(data, 0, sizeof(SemaphoreData) + (nsems-1)*sizeof(int));
    }
    if (key == IPC_PRIVATE) { 
	name = NULL;
    } else if (key <= ftokTableUsed) { 
	sprintf(buf, "%s.mutex", ftokTable[key-1]);
	name = buf;
    } else { 
	sprintf(buf, "key.%ld.mutex", key);
	name = buf;
    }
    mutex = CreateMutex(&inheritable, 0, name);
    if (mutex == NULL) { 
	ERRNO();
	UnmapViewOfFile(data);
	CloseHandle(shmem);
	if (f != INVALID_HANDLE_VALUE) {
	    CloseHandle(f);
	}
	return -1;
    }

    if (key == IPC_PRIVATE) { 
	name = NULL;
    } else if (key <= ftokTableUsed) { 
	sprintf(buf, "%s.event", ftokTable[key-1]);
	name = buf;
    } else { 
	sprintf(buf, "key.%ld.event", key);
	name = buf;
    }
    event = CreateEvent(&inheritable, 1, 0, name);
    if (event == NULL) { 
	ERRNO();
	UnmapViewOfFile(data);
	CloseHandle(shmem);
	CloseHandle(mutex);
	if (f != INVALID_HANDLE_VALUE) {
	    CloseHandle(f);
	}
	return -1;
    }
    
    if (key == IPC_PRIVATE) { 
	name = NULL;
    } else if (key <= ftokTableUsed) { 
	sprintf(buf, "%s.sem", ftokTable[key-1]);
	name = buf;
    } else { 
	sprintf(buf, "key.%ld.sem", key);
	name = buf;
    }
    sem = CreateSemaphore(&inheritable, 0, 0xFFFF, name);
    if (sem == NULL) { 
	ERRNO();
	UnmapViewOfFile(data);
	CloseHandle(shmem);
	CloseHandle(mutex);
	CloseHandle(event);
	if (f != INVALID_HANDLE_VALUE) {
	    CloseHandle(f);
	}
	return -1;
    }
    
    desc = (SemaphoreDescriptor*)malloc(sizeof(SemaphoreDescriptor));
    desc->mutex = mutex;
    desc->event = event;
    desc->sem   = sem;
    desc->shmem = shmem;
    desc->data  = data;
    desc->nSems = nsems;
    desc->fileName = fileName;
    desc->fh = f;
    return (int)desc;
}

int semop(int semid, struct sembuf *sops, unsigned nsops)
{    
    int i;
    SemaphoreDescriptor* desc = (SemaphoreDescriptor*)semid;
    
    while (1) { 
	/*printf("Locking mutex\n");*/
	if (WaitForSingleObject(desc->mutex, INFINITE) != WAIT_OBJECT_0) { 	
	    ERRNO();
	    return -1;
	}
	desc->data->waitCount += 1;
	/*printf("Mutex locked, wait count=%d\n", desc->data->waitCount);*/
	for (i = 0; i < nsops; i++) { 
	    /*printf("sops[i].sem_num=%d, sops[i].sem_op=%d, desc->data->value[sops[i].sem_num]=%d\n", sops[i].sem_num, sops[i].sem_op, desc->data->value[sops[i].sem_num]);*/
	    if ((unsigned)sops[i].sem_num >= desc->nSems) { 
		/*printf("desc->nSems=%d\n", desc->nSems);*/
		while (--i >= 0) { 
		    desc->data->value[sops[i].sem_num] -= sops[i].sem_op;
		}
		SET_ERRNO(ERANGE);
		desc->data->waitCount -= 1;
		ReleaseMutex(desc->mutex);
		return -1;
	    }
	    if (sops[i].sem_op < 0) { 
		if (desc->data->value[sops[i].sem_num] < -sops[i].sem_op) { 
		    while (--i >= 0) { 
			desc->data->value[sops[i].sem_num] -= sops[i].sem_op;
		    }
		    if (sops[i].sem_flg & IPC_NOWAIT) {
			SET_ERRNO(EAGAIN);
			desc->data->waitCount -= 1;
			ReleaseMutex(desc->mutex);
			return -1;
		    }
		    break;
		}
		desc->data->value[sops[i].sem_num] += sops[i].sem_op;
	    } else if (sops[i].sem_op == 0) { 
		if (desc->data->value[sops[i].sem_num] != 0) { 
		    while (--i >= 0) { 
			desc->data->value[sops[i].sem_num] -= sops[i].sem_op;
		    }
		    if (sops[i].sem_flg & IPC_NOWAIT) {
			SET_ERRNO(EAGAIN);
			desc->data->waitCount -= 1;
			ReleaseMutex(desc->mutex);
			return -1;
		    }
		    break;
		}
	    } else { 
		desc->data->value[sops[i].sem_num] += sops[i].sem_op;
	    }
	}
	if (i == nsops) { 
	    /*printf("Operation succeeded, desc->data->waitCount=%d\n", desc->data->waitCount);*/
	    if (--desc->data->waitCount != 0) { 	
		SetEvent(desc->event);
		do { 
		    /*printf("Waiting for semaphore\n");*/
		    if (WaitForSingleObject(desc->sem, INFINITE) != WAIT_OBJECT_0) {
			/*printf("Wait for semaphore failed\n");*/
			while (--i >= 0) { 
			    desc->data->value[sops[i].sem_num] -= sops[i].sem_op;
			}
			ERRNO();
			ReleaseMutex(desc->mutex);
			return -1;
		    }
		    /*printf("Waiting for semaphore succeeded\n");*/
		} while (--desc->data->waitCount != 0);
		ResetEvent(desc->event);
	    }
	    ReleaseMutex(desc->mutex);
	    return 0;
	} else { 
	    /*printf("Operation failed, i=%d\n", i);*/
	    ReleaseMutex(desc->mutex);
	    if (WaitForSingleObject(desc->event, INFINITE) != WAIT_OBJECT_0) {
		/*printf("Wait event failed\n");*/
		ERRNO();
		return -1;
	    }
	    /*printf("Event signaled\n");*/
	    ReleaseSemaphore(desc->sem, 1, NULL);
	}
    }
}

int semctl(int semid, int semnum, int cmd, union semun arg)
{
    int i;
    SemaphoreDescriptor* desc = (SemaphoreDescriptor*)semid;
    switch (cmd) { 
      case IPC_STAT:	
	SET_ERRNO(ENOSYS); 
	return -1; /* not implemented */
      case IPC_SET:
	SET_ERRNO(ENOSYS); 
	return -1; /* not implemented */
      case IPC_RMID:
	UnmapViewOfFile(desc->data);
	CloseHandle(desc->sem);
	CloseHandle(desc->event);
	CloseHandle(desc->shmem);
	CloseHandle(desc->mutex);
	if (desc->fh != INVALID_HANDLE_VALUE) { 
	    CloseHandle(desc->fh);
	    if (desc->fileName != NULL) { 
		DeleteFile(desc->fileName);
		free(desc->fileName);
	    }
	}
	free(desc);
	return 0;
      case GETALL:
	if (WaitForSingleObject(desc->mutex, INFINITE) != WAIT_OBJECT_0) { 
	    return -1;
	}
	for (i = 0; i < desc->nSems; i++) { 
	    arg.array[i] = desc->data->value[i];
	}
	ReleaseMutex(desc->mutex);
	return 0;
      case GETNCNT:
	SET_ERRNO(ENOSYS); 
	return -1; /* not implemented */
      case GETPID:
	SET_ERRNO(ENOSYS); 
	return -1; /* not implemented */
      case GETVAL:	
	return (unsigned)semnum < (unsigned)desc->nSems ? desc->data->value[semnum] : -1;
      case GETZCNT:
	return -1; /* not implemented */
      case SETALL:
	if (WaitForSingleObject(desc->mutex, INFINITE) != WAIT_OBJECT_0) { 
	    ERRNO();
	    return -1;
	}
	for (i = 0; i < desc->nSems; i++) { 
	    desc->data->value[i] = arg.array[i];
	}
	if (desc->data->waitCount != 0) { 
	    PulseEvent(desc->event);
	    do { 
		if (WaitForSingleObject(desc->sem, INFINITE) != WAIT_OBJECT_0) {
		    ERRNO();
		    ReleaseMutex(desc->mutex);
		    return -1;
		}
	    } while (--desc->data->waitCount != 0);
	}
	ReleaseMutex(desc->mutex);
	return 0;
      case SETVAL:	
	if ((unsigned)semnum >= (unsigned)desc->nSems) { 
	    return -1;
	}
	if (WaitForSingleObject(desc->mutex, INFINITE) != WAIT_OBJECT_0) { 
	    ERRNO();
	    return -1;
	}
	desc->data->value[semnum] = arg.val;
	if (desc->data->waitCount != 0) { 
	    SetEvent(desc->event);
	    do { 
		if (WaitForSingleObject(desc->sem, INFINITE) != WAIT_OBJECT_0) {
		    ERRNO();
		    ReleaseMutex(desc->mutex);
		    return -1;
		}
	    } while (--desc->data->waitCount != 0);
	    ResetEvent(desc->event);
	}
	ReleaseMutex(desc->mutex);
	return 0;
    }
    SET_ERRNO(EINVAL); 
    return -1;
}
