#ifndef _LOCK_MEM_MNG_H_
#define _LOCK_MEM_MNG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"

#define MAX_MEM_MANAGER (128)

typedef struct {
    LIST list;
    /* data */
    void * memAddr;
} DEAD_LOCK_MEM_INFO;

void LockMemInit(size_t everyMemSize);
void * AllocMemUnit(void);
void FreeMemUnit(void *memAddr);
int getMemFreeCount(void);
int getMemAllocCount(void);

#ifdef __cplusplus
}
#endif
#endif
