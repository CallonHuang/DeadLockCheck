#ifndef _DEAD_LOCK_EXTERNAL_H_
#define _DEAD_LOCK_EXTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>

int LockWithRecord(pthread_mutex_t *lockAddr, pid_t pid, int lockType, struct timespec *timeOut);
int UnlockWithRecord(pthread_mutex_t *lockAddr, pid_t pid);

void PrtRecord(void);

#define pthread_mutex_lock(lockAddr)               LockWithRecord(lockAddr, syscall(SYS_gettid), 0, 0)
#define pthread_mutex_timedlock(lockAddr, timeout) LockWithRecord(lockAddr, syscall(SYS_gettid), 1, timeout)
#define pthread_mutex_trylock(lockAddr)            LockWithRecord(lockAddr, syscall(SYS_gettid), 2, 0)

#define pthread_mutex_unlock(lockAddr) UnlockWithRecord(lockAddr, syscall(SYS_gettid))

#ifdef __cplusplus
}
#endif

#endif
