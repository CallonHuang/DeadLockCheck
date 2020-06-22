#include "memCache.h"
#include <malloc.h>
#include <stdlib.h>
#include <pthread.h>

static MEM_MANAGER_INFO memAlloc;
static MEM_MANAGER_INFO memFree;
static pthread_mutex_t memListMutex = PTHREAD_MUTEX_INITIALIZER;
static void *memStart;
static size_t memUnitSize = 0;

void memMngInit(size_t unitSize)
{
    int i = 0;
    ListInit(&memFree.list);
    ListInit(&memAlloc.list);
    memStart = malloc(unitSize * MAX_MEM_MANAGER);
    for (i = 0; i < MAX_MEM_MANAGER; i++) {
        MEM_MANAGER_INFO *newNode;
        newNode = (MEM_MANAGER_INFO *)malloc (sizeof(MEM_MANAGER_INFO));
        newNode->memAddr = memStart + i * unitSize;
        ListAddTail(&memFree.list, (NODE *)newNode);
    }
    memUnitSize = unitSize;
}

void *AllocMemUnit()
{
    MEM_MANAGER_INFO *memInfo = NULL;
    void *retAddr = NULL;
    pthread_mutex_lock(&memListMutex);
    if (memFree.list.count > 0) {
        memInfo = (MEM_MANAGER_INFO *)memFree.list.node.next;
        ListDelete(&memFree.list, (NODE *)memInfo);
        ListAddTail(&memAlloc.list, (NODE *)memInfo);
        pthread_mutex_unlock(&memListMutex);
        retAddr = memInfo->memAddr;
    } else {
        pthread_mutex_unlock(&memListMutex);
        retAddr = malloc(memUnitSize);
    }
    
    return retAddr;
}

void FreeMemUnit(void *memAddr)
{
    MEM_MANAGER_INFO *currNode;
    if (memAddr >= memStart && memAddr <= memStart + (MAX_MEM_MANAGER - 1) * memUnitSize) {
        pthread_mutex_lock(&memListMutex);
        LIST_FOR_EACH(MEM_MANAGER_INFO, currNode, memAlloc.list) {
            if (currNode->memAddr == memAddr) {
                ListDelete(&memAlloc.list, (NODE *)currNode);
                ListAddTail(&memFree.list, (NODE *)currNode);
                break;
            }
        }
        pthread_mutex_unlock(&memListMutex);
    } else {
        free(memAddr);
    }
}

int getMemFreeCount()
{
    return memFree.list.count;
}

int getMemAllocCount()
{
    return memAlloc.list.count;
}
