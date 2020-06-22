#ifndef _LOCK_MEM_MNG_H_
#define _LOCK_MEM_MNG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"

#define MAX_MEM_MANAGER (1<<6)

typedef struct {
    LIST list;
    /* data */
    void *memAddr;
} MEM_MANAGER_INFO;

void memMngInit(size_t unitSize);
void *AllocMemUnit(void);
void FreeMemUnit(void *memAddr);
int getMemFreeCount(void);
int getMemAllocCount(void);

#ifdef __cplusplus
}
#endif
#endif
