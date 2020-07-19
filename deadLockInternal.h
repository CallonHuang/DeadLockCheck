#ifndef _DEAD_LOCK_INTERNAL_H_
#define _DEAD_LOCK_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hashList.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define MAX_HASH_BITS  (6)
#define MAX_REQUEST_TABLE  (1<<MAX_HASH_BITS)
#define MAX_OWNER_TABLE (1<<MAX_HASH_BITS)
/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 (0x9e370001UL)

#define UNSPECIFIED_PID (-1)
#define OK              (0)
#define NOT_FOUND       (404)
#define REVISITED       (-1)
#define VISITED_BEFORE  (-2)

#define VISIT_BITMASK    (1L)
#define VISIT_ID_OFFSET (4)

typedef enum {
    UNVISITED = 0,
    VISITED = 1
} VISIT_E;

typedef struct {
    HASH_NODE hashNode;
    int visitTag;
    /* data */
    void *lockAddr;
    pid_t pid;
    void *retFuncAddr;
} DEAD_LOCK_INFO;

static inline unsigned int Hash32(unsigned long val, unsigned int bits)
{
    /* On some cpus multiply is faster, on others gcc will do shifts */
    unsigned int hash = val * GOLDEN_RATIO_PRIME_32;

    /* High bits are more random, so use them. */
    return hash >> (32 - bits);
}

static void DeadLockCheckInit(void) __attribute__((constructor));

#ifdef __cplusplus
}
#endif
#endif
