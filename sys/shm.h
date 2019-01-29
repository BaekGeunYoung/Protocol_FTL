#ifndef _SYS_SHM_H
#define _SYS_SHM_H	

#include "ipc.h"

/* Permission flag for shmget.  */
#define SHM_R           0400            /* or S_IRUGO from <linux/stat.h> */
#define SHM_W           0200            /* or S_IWUGO from <linux/stat.h> */

/* Flags for `shmat'.  */
#define SHM_RDONLY      010000          /* attach read-only else read-write */
#define SHM_RND         020000          /* round attach address to SHMLBA */
#define SHM_REMAP       040000          /* take-over region on attach */

/* Commands for `shmctl'.  */
#define SHM_LOCK        11              /* lock segment (root only) */
#define SHM_UNLOCK      12              /* unlock segment (root only) */

 
/* Permission flag for shmget.  */
#define SHM_R           0400            /* or S_IRUGO from <linux/stat.h> */
#define SHM_W           0200            /* or S_IWUGO from <linux/stat.h> */

/* Flags for `shmat'.  */
#define SHM_RDONLY      010000          /* attach read-only else read-write */
#define SHM_RND         020000          /* round attach address to SHMLBA */
#define SHM_REMAP       040000          /* take-over region on attach */

/* Commands for `shmctl'.  */
#define SHM_LOCK        11              /* lock segment (root only) */
#define SHM_UNLOCK      12              /* unlock segment (root only) */

struct shmid_ds;
extern int shmctl (int __shmid, int __cmd, struct shmid_ds *__buf);

/* Get shared memory segment.  */
extern int shmget (key_t __key, size_t __size, int __shmflg);

/* Attach shared memory segment.  */
extern void *shmat (int __shmid, const void *__shmaddr, int __shmflg);

/* Detach shared memory segment.  */
extern int shmdt (const void *__shmaddr);

#endif /* sys/shm.h */

