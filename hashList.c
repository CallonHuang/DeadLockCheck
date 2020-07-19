#include "hashList.h"
/*
                                                hash_node
                            hash_node            =====
                             =====      /------->node2
                   /-------->node1      |        =====
        hashHead  |         =====      |        next------->NULL
        =========  |  /----> next-------/        =====
/------>first------/  |      =====           /---pprev
|       =========     |   /--pprev           |   =====
|                     |   |  =====           |
|                     \---|------------------/
|                         |
\-------------------------/

*/
void HListInit(HASH_LIST *list)
{
    list->hashHead.first = NULL;
    list->count = 0;
}

void HListAddHead(HASH_LIST *list, HASH_NODE *node)
{
    node->next = list->hashHead.first;
    if (NULL != list->hashHead.first)
        list->hashHead.first->pprev = &node->next;
    list->hashHead.first = node;
    node->pprev = &list->hashHead.first;
    list->count++;
}

void HListDelete(HASH_LIST *list, HASH_NODE *node)
{
    *node->pprev = node->next;
    if (NULL != node->next)
        node->next->pprev = node->pprev;
    list->count--;
}

void HListDestroy(HASH_LIST *list)
{
    HASH_NODE *node;
    for (node = list->hashHead.first; node != NULL; node = node->next) {
        HLIST_FREE_NODE(list, node);
    }
}
