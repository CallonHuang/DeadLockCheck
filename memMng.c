#include "memMng.h"
#include <malloc.h>
#include <stdlib.h>
#include <pthread.h>

static DEAD_LOCK_MEM_INFO memAlloc;
static DEAD_LOCK_MEM_INFO memFree;
static pthread_mutex_t memListMutex = PTHREAD_MUTEX_INITIALIZER;
static void *memStart;
static size_t memUnitSize = 0;

void memMngInit(size_t everyMemSize)
{
    int i = 0;
    ListInit(&memFree.list);
    ListInit(&memAlloc.list);
    memStart = malloc(everyMemSize * MAX_MEM_MANAGER);
    for (i = 0; i < MAX_MEM_MANAGER; i++) {
        DEAD_LOCK_MEM_INFO* newNode;
        newNode = (DEAD_LOCK_MEM_INFO *)malloc (sizeof(DEAD_LOCK_MEM_INFO));
        newNode->memAddr = memStart+i*everyMemSize;
        ListAddTail(&memFree.list, (NODE *)newNode);
    }
    memUnitSize = everyMemSize;
}

void * AllocMemUnit()
{
    DEAD_LOCK_MEM_INFO *memInfo = NULL;
    void * retAddr = NULL;
    pthread_mutex_lock(&memListMutex);
    if (memFree.list.count > 0) {
        memInfo = (DEAD_LOCK_MEM_INFO *)memFree.list.node.next;
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
    DEAD_LOCK_MEM_INFO *currNode;
    if (memAddr >= memStart && memAddr <= memStart+(MAX_MEM_MANAGER-1)*memUnitSize) {
        pthread_mutex_lock(&memListMutex);
        LIST_FOR_EACH(DEAD_LOCK_MEM_INFO, currNode, memAlloc.list) {
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
