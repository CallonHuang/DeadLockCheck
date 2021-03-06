#include "deadLockInternal.h"
#include "memUnitCache.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h>

static HASH_LIST requestTable[MAX_REQUEST_TABLE];
static HASH_LIST ownerTable[MAX_OWNER_TABLE];
static pthread_mutex_t lockRecordMutex = PTHREAD_MUTEX_INITIALIZER;

static void DeadLockCheckInit(void)
{
    int i = 0;
    memset(requestTable, 0, sizeof(requestTable));
    memset(ownerTable, 0, sizeof(ownerTable));
    for (i = 0; i < MAX_REQUEST_TABLE; i++)
        HListInit(&requestTable[i]);
    for (i = 0; i < MAX_OWNER_TABLE; i++)
        HListInit(&ownerTable[i]);
    memUnitCacheInit(sizeof(DEAD_LOCK_INFO));
}

/*brief: record the lock info to requestTable*/
static void BeforeLock(pthread_mutex_t *lockAddr, pid_t pid, void *retFuncAddr)
{
    DEAD_LOCK_INFO *newNode;
    unsigned int requestIndex;
    newNode = (DEAD_LOCK_INFO *)AllocMemUnit();
    requestIndex = Hash32((unsigned long)pid, MAX_HASH_BITS);
    newNode->lockAddr = (void *)lockAddr;
    newNode->pid = pid;
    newNode->retFuncAddr = retFuncAddr;
    pthread_mutex_lock(&lockRecordMutex);
    HListAddHead(&requestTable[requestIndex], (HASH_NODE *)newNode);
    pthread_mutex_unlock(&lockRecordMutex);
}

/*brief: clear the lock info in requestTable, record the lock info to ownerTable*/
static void AfterLock(pthread_mutex_t *lockAddr, pid_t pid, void *retFuncAddr)
{
    DEAD_LOCK_INFO *newNode, *currNode;
    unsigned int requestIndex = Hash32((unsigned long)pid, MAX_HASH_BITS);
    unsigned int ownerIndex = Hash32((unsigned long)lockAddr, MAX_HASH_BITS);
    newNode = (DEAD_LOCK_INFO *)AllocMemUnit();
    newNode->pid = pid;
    newNode->lockAddr = (void *)lockAddr;
    newNode->retFuncAddr = retFuncAddr;
    pthread_mutex_lock(&lockRecordMutex);
    HListAddHead(&ownerTable[ownerIndex], (HASH_NODE *)newNode);
    HLIST_FOR_EACH(DEAD_LOCK_INFO, currNode, requestTable[requestIndex]) {
        if (currNode->lockAddr == lockAddr && currNode->pid == pid)
            break;
    }
    if (currNode != NULL) {
        HListDelete(&requestTable[requestIndex], (HASH_NODE *)currNode);
        pthread_mutex_unlock(&lockRecordMutex);
        FreeMemUnit(currNode);
    } else {/*if not found*/
        pthread_mutex_unlock(&lockRecordMutex);
    }
    
}

int LockWithRecord(pthread_mutex_t *lockAddr, pid_t pid, int lockType, struct timespec *timeOut)
{
    int ret;  
    void *retFuncAddr = __builtin_return_address(0);
    BeforeLock(lockAddr, pid, retFuncAddr);
    if (2 == lockType)
        ret = pthread_mutex_trylock(lockAddr);
    else if (1 == lockType)
        ret = pthread_mutex_timedlock(lockAddr, timeOut);
    else
        ret = pthread_mutex_lock(lockAddr);

    if (OK != ret) {
        DEAD_LOCK_INFO *currNode;
        unsigned int requestIndex = Hash32((unsigned long)pid, MAX_HASH_BITS);
        pthread_mutex_lock(&lockRecordMutex);
        /*clear the node that insert by [BeforeLock] just now*/
        HLIST_FOR_EACH(DEAD_LOCK_INFO, currNode, requestTable[requestIndex]) {
            if (currNode->lockAddr == lockAddr && currNode->pid == pid)
                break;
        }
        if (currNode != NULL) {
            HListDelete(&requestTable[requestIndex], (HASH_NODE *)currNode);
            pthread_mutex_unlock(&lockRecordMutex);
            FreeMemUnit(currNode);
        } else {/*if not found*/
            pthread_mutex_unlock(&lockRecordMutex);
        }
        
    } else {
        AfterLock(lockAddr, pid, retFuncAddr);
    }
    
    return ret;
}

/*brief: clear the lock info in ownerTable*/
static void AfterUnlock(pthread_mutex_t *lockAddr, pid_t pid, void *retFuncAddr)
{
    DEAD_LOCK_INFO *currNode;
    unsigned int ownerIndex = Hash32((unsigned long)lockAddr, MAX_HASH_BITS);
    pthread_mutex_lock(&lockRecordMutex);
    HLIST_FOR_EACH(DEAD_LOCK_INFO, currNode, ownerTable[ownerIndex]) {
        if (currNode->pid == pid && currNode->lockAddr == lockAddr)
            break;
    }
    if (currNode != NULL) {
        HListDelete(&ownerTable[ownerIndex], (HASH_NODE *)currNode);
        pthread_mutex_unlock(&lockRecordMutex);
        FreeMemUnit(currNode);
    } else {/*if not found*/
        pthread_mutex_unlock(&lockRecordMutex);
    }
    
}

int UnlockWithRecord(pthread_mutex_t *lockAddr, pid_t pid)
{
    void *retFuncAddr = __builtin_return_address(0);
    int ret = pthread_mutex_unlock(lockAddr);
    if (OK == ret)
        AfterUnlock(lockAddr, pid, retFuncAddr);
    return ret;
}

/*insert a node to traceStackList, that record your access footprint*/
static void InsertStack(HASH_LIST *traceStackList, pid_t pid, void *lockAddr, void * retFuncAddr) 
{
    DEAD_LOCK_INFO *newNode = NULL;
    newNode = (DEAD_LOCK_INFO *)malloc(sizeof(DEAD_LOCK_INFO));
    newNode->pid = pid;
    newNode->retFuncAddr = retFuncAddr;
    newNode->lockAddr = lockAddr;
    HListAddHead(traceStackList, (HASH_NODE *)newNode);
}

/*initialize all of node to be not visited*/
static void ClearRequestVisit()
{
    int requestIndex = 0;
    DEAD_LOCK_INFO *currNode = NULL;
    for (requestIndex = 0; requestIndex < MAX_REQUEST_TABLE; requestIndex++) {
        if (requestTable[requestIndex].count != 0) {
            HLIST_FOR_EACH(DEAD_LOCK_INFO, currNode, requestTable[requestIndex]) {
                currNode->visitTag = UNVISITED;
            }
        }
    }
}

/*
    *part of algorithm to find lock circle
    *brief: select a request list which has some node not visited.
    *param:
    *    [in]visitArray : for [SelectRequest] run fast, visitArray can sign the number of visited node(of the index of request list)
    *return:
    *    requestIndex : the index of request list which has some node not be visited 
*/
static int SelectRequest(int *visitArray)
{
    int requestIndex = 0;
    for (requestIndex = 0; requestIndex < MAX_REQUEST_TABLE; requestIndex++) {
        if (requestTable[requestIndex].count != 0 && visitArray[requestIndex] < requestTable[requestIndex].count)
            break;
    }
    return requestIndex;
}

/*
    *part of algorithm to find lock circle
    *brief: visit the specified request list, you will get return 'REVISITED'/'NOT_FOUND'/'OK', 'REVISITED' means you turned back
    *param:
    *    [in]requestIndex   : sign the index of request list that you should visit
    *    [in]pid            : sign the pid that you need find, if 'UNSPECIFIED_PID', you can ignore it
    *    [in&out]visitArray : for [SelectRequest] run fast, visitArray can sign the number of visited node(of the index of request list)
    *    [out]targetNode    : the pointer of the target that meet your condition in the request list
    *    [in]visitID       : to sign every [RequestTrace], use to decide return 'VISITED_BEFORE' or 'REVISITED'
    *return:
    *    'NOT_FOUND'      : the owner is not requesting now / no more unvisited node of the index of request list
    *    'REVISITED'      : in the same [RequestTrace], you returned the node that you visit before
    *    'VISITED_BEFORE' : in the different [RequestTrace], you returned the node that you visit before
    *    'OK'             : visit a unvisited node in the request list successfully
*/
static int VisitRequest(int requestIndex, pid_t pid, int *visitArray, DEAD_LOCK_INFO **targetNode, int visitID)
{
    int ret = NOT_FOUND;
    HLIST_FOR_EACH(DEAD_LOCK_INFO, (*targetNode), requestTable[requestIndex]) {
        if (UNSPECIFIED_PID == pid) {
            ret = NOT_FOUND;
            /*must be has unvisited node*/
            if (UNVISITED == ((*targetNode)->visitTag & VISIT_BITMASK)) {
                (*targetNode)->visitTag = (visitID << VISIT_ID_OFFSET | VISIT_BITMASK);
                break;
            }
        } else {
            if ((*targetNode)->pid == pid) {
                if (UNVISITED == ((*targetNode)->visitTag & VISIT_BITMASK)) {
                    (*targetNode)->visitTag = (visitID << VISIT_ID_OFFSET | VISIT_BITMASK);
                } else {
                    if ((*targetNode)->visitTag >> VISIT_ID_OFFSET == visitID)
                        ret = REVISITED;
                    else
                        ret = VISITED_BEFORE;
                }
                break;
            } else {
                ret = NOT_FOUND;
            }
        }
        
    }
    /*find the last but 'NOT_FOUND' or returned the node that you visit before*/
    if (*targetNode == NULL || 0 > ret)
        return ret;

    visitArray[requestIndex]++;
    return OK;
}

/*
    *part of algorithm to find lock circle
    *brief: traverse request list and owner list, to find the relation path
    *param:
    *    [in]requestIndex    : [SelectRequest] returned the index of request list which has some node not be visited
    *    [in&out]visitArray  : for [SelectRequest] run fast, visitArray can sign the number of visited node(of the index of request list)
    *    [in]visitID        : to sign every [RequestTrace], if [VisitRequest] returned 'VISITED_BEFORE', means you returned the node that you visit in before [RequestTrace]
    *return: 
    *    'NOT_FOUND'      : the owner is not requesting now / no more unvisited node of the index of request list
    *    'VISITED_BEFORE' : in the different [RequestTrace], you returned the node that you visit before
    *    'REVISITED'      : in the same [RequestTrace], you returned the node that you visit before
*/
static int RequestTrace(int requestIndex, int *visitArray, int visitID)
{
    DEAD_LOCK_INFO *targetNode = NULL, *currNode = NULL;
    HASH_LIST traceStackList;
    int ownerIndex = 0, ret = OK;
    pid_t pid = UNSPECIFIED_PID;
    HListInit(&traceStackList);
    do {
        if (OK != (ret = VisitRequest(requestIndex, pid, visitArray, &targetNode, visitID))) {
            if (UNSPECIFIED_PID == pid) {
                /*to prevent you use the same requestIndex again and again*/
                visitArray[requestIndex] = requestTable[requestIndex].count;
                break;
            }
            if (NULL != currNode)
                InsertStack(&traceStackList, currNode->pid, currNode->lockAddr, currNode->retFuncAddr);
            if (NOT_FOUND == ret || VISITED_BEFORE == ret)
                printf("request wait owner as below:\n");
            else
                printf("find lock circle as below:\n");
            HLIST_FOR_EACH(DEAD_LOCK_INFO, currNode, traceStackList) {
                if (currNode != (void *)traceStackList.hashHead.first)
                    printf("->");
                printf("[pid: %d func: %p]", currNode->pid, currNode->retFuncAddr);
            }
            printf("\n");
            break;
        }
        InsertStack(&traceStackList, targetNode->pid, targetNode->lockAddr, targetNode->retFuncAddr);
        ownerIndex = Hash32((unsigned long)targetNode->lockAddr, MAX_HASH_BITS);
        HLIST_FOR_EACH(DEAD_LOCK_INFO, currNode, ownerTable[ownerIndex]) {
            if (currNode->lockAddr == targetNode->lockAddr)
                break;
        }
        
        if (currNode != (DEAD_LOCK_INFO *)&ownerTable[ownerIndex].hashHead.first) {
            pid = currNode->pid;
            requestIndex = Hash32((unsigned long)currNode->pid, MAX_HASH_BITS);
        } else {
            /*request is valid, no owner exist*/
            ret = NOT_FOUND;
            break;
        }
    } while (1);
    HListDestroy(&traceStackList);
    return ret;
}

static void PrtTable(HASH_LIST *table, unsigned int size)
{
    DEAD_LOCK_INFO *currNode;
    int Index = 0;
    for (Index = 0; Index < size; Index++) {
        if (table[Index].count != 0) {
            printf("[%d] ", Index);
            HLIST_FOR_EACH(DEAD_LOCK_INFO, currNode, table[Index]) {
                printf("|pid: %d addr: %p func: %p|", currNode->pid, currNode->lockAddr, currNode->retFuncAddr);
            }
            printf("\n");
        }
    }
}

void PrtRecord()
{
    int requestIndex = 0 ,visitID = 0;
    /*array of each index of requestTable visited number, to sel requestIndex quickly*/
    int visitArray[MAX_REQUEST_TABLE] = {0};
    printf("-----------------------------------------\n");
    printf("free rest count: %d, alloc used count: %d\n", getMemFreeCount(), getMemAllocCount());
    ClearRequestVisit();
    printf("-----------------------------------------\n");
    printf("request[key:pid] As Bellow:\n");
    PrtTable(requestTable, MAX_REQUEST_TABLE);
    printf("owner[key:addr] As Bellow:\n");
    PrtTable(ownerTable, MAX_OWNER_TABLE);
    printf("-----------------------------------------\n");
    printf("#########################################\n");
    /*algorithm to find lock circle*/
    do {
        /*select requestIndex which has unvisited node*/
        requestIndex = SelectRequest(visitArray);
        if (requestIndex == MAX_REQUEST_TABLE) {
            break;
        }
        visitID++;
        (void)RequestTrace(requestIndex, visitArray, visitID);
    } while (1);
    printf("#########################################\n");
}
