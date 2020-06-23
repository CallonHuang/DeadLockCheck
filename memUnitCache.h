#ifndef _MEM_CACHE_H_
#define _MEM_CACHE_H_

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

void memUnitCacheInit(size_t unitSize);
void *AllocMemUnit(void);
void FreeMemUnit(void *memAddr);
int getMemFreeCount(void);
int getMemAllocCount(void);

#ifdef __cplusplus
}
#endif
#endif
