#ifndef _HASH_LIST_H_
#define _HASH_LIST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct hash_node {
    struct hash_node *next;
    struct hash_node **pprev;
} HASH_NODE;

typedef struct {
    HASH_NODE *first;
} HASH_HEAD;

typedef struct
{
    HASH_HEAD hashHead;
    int count;
} HASH_LIST;

#define HLIST_FOR_EACH(TYPE, curr, list_struct)  \
for (curr = (TYPE *)list_struct.hashHead.first; curr != NULL; curr = (void *)curr->hashNode.next)

#define HLIST_FREE_NODE(list, node) do {                                      \
                                        HListDelete(list, (HASH_NODE *)node); \
                                        free(node);                           \
                                    } while(0)

void HListInit(HASH_LIST *list);
void HListAddHead(HASH_LIST *list, HASH_NODE *node);
void HListDelete(HASH_LIST *list, HASH_NODE *node);
void HListDestroy(HASH_LIST *list);

#ifdef __cplusplus
}
#endif
#endif
