#ifndef _SYS_SEM_H
#define _SYS_SEM_H	1

#include "ipc.h"

/* Flags for `semop'.  */
#define SEM_UNDO        0x1000          /* undo the operation on exit */

/* Commands for `semctl'.  */
#define GETPID          11              /* get sempid */
#define GETVAL          12              /* get semval */
#define GETALL          13              /* get all semval's */
#define GETNCNT         14              /* get semncnt */
#define GETZCNT         15              /* get semzcnt */
#define SETVAL          16              /* set semval */
#define SETALL          17              /* set all semval's */

/* Structure used for argument to `semop' to describe operations.  */
struct sembuf
{
  unsigned short int sem_num;	/* semaphore number */
  short int sem_op;		/* semaphore operation */
  short int sem_flg;		/* operation flag */
};

union semun
{
    int val;                  
    struct semid_ds *buf;     
    unsigned short int *array;
    struct seminfo *__buf;    
};


/* Semaphore control operation.  */
extern int semctl (int __semid, int __semnum, int __cmd, union semun arg);

/* Get semaphore.  */
extern int semget (key_t __key, int __nsems, int __semflg);

/* Operate on semaphore.  */
extern int semop (int __semid, struct sembuf *__sops, size_t __nsops);

#endif /* sys/sem.h */





