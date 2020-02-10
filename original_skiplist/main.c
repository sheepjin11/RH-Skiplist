#include <stdio.h>
#include <libpmemobj.h>
#include "/home/shrimp1/computer_archi/pmdk-1.7/src/examples/libpmemobj/list_map/skiplist_map.h"
#include "unistd.h"
#include <assert.h>

int main()
{
    PMEMobjpool* pop=NULL;
    if(access("/mnt/pmem/test", F_OK) ==-1)
    {
    if((pop= pmemobj_create("/mnt/pmem/test", POBJ_LAYOUT_NAME(test_), 0, 0666))==NULL)
    {
        perror("failed to create pool\n");
        exit(0);
    }
    }
    else
    {
        pop=pmemobj_open("/mnt/pmem/test", POBJ_LAYOUT_NAME(test_));
        if(pop==NULL)
        {
            perror("failed to open pool\n");
            exit(0);
        }
    }
    assert(pop!=NULL);
    int i=0;
    TOID(struct skiplist_map_node) map;
    int create= skiplist_map_create(pop, &map, NULL);
    printf("create is %d\n", create);
    for(i=1;i<1000000;i++)
    {
        printf("key is :%d\n", i);
        skiplist_map_insert(pop, map, i, i);
    }
    pmemobj_close(pop);
}

