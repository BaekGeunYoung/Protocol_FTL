#ifndef _SYS_IPC_H
#define _SYS_IPC_H	1

/* Bits for `msgget', `semget', and `shmget'.  */
#define IPC_CREAT       01000           /* Create key if key does not exist. */
#define IPC_EXCL        02000           /* Fail if key exists.  */
#define IPC_NOWAIT      04000           /* Return error on wait.  */

/* Control commands for `msgctl', `semctl', and `shmctl'.  */
#define IPC_RMID        0               /* Remove identifier.  */
#define IPC_SET         1               /* Set `ipc_perm' options.  */
#define IPC_STAT        2               /* Get `ipc_perm' options.  */
#ifdef __USE_GNU
# define IPC_INFO       3               /* See ipcs.  */
#endif

/* Special key values.  */
#define IPC_PRIVATE     0   /* Private key.  */

typedef long int key_t;


extern key_t ftok(char *pathname, char proj);


#endif /* sys/ipc.h */
